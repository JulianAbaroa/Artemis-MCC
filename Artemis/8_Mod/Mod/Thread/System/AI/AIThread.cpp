module Mod.Thread.System;
import :AI;

import Engine.Base.Lifecycle.Type;
import Bridge.Codec.Flat.Type;
import std;

using namespace std::chrono_literals;

namespace
{
    using Engine::Base::Lifecycle::Type::Status;
    using Bridge::Codec::Flat::Type::FlatRequest;

    using std::chrono::steady_clock;
    using std::chrono::nanoseconds;

    // Sweeps at or above this are logged with the time of each stage.
    constexpr nanoseconds k_SlowSweep{ 50'000'000 };
    // Stages below this are left out of the slow sweep log.
    constexpr std::uint64_t k_MinReportedStageNs{ 100'000 };
}

namespace Mod::Thread::System
{
    auto AIThread::AddObserver(std::function<void(const ObservedTick&)> observer) -> void
    {
        m_Observers.push_back(std::move(observer));
    }

    auto AIThread::Run() -> void
    {
        auto& logs{ m_Service.m_Log };
        auto& telemetry{ m_Service.m_TelemetryStore };
        auto& lifecycle{ m_Base.m_LifecycleStore };

        logs.Message("[AIThread] INFO: Started.");

        while (lifecycle.IsRunning())
        {
            const Status status{ lifecycle.GetStatus() };

            // The game thread waits for each tick read only while this thread is consuming ticks.
            lifecycle.SetReaderActive(status == Status::Running);

            switch (status)
            {
            case Status::Waiting:
            case Status::Destroyed:
                lifecycle.WaitForBlam();
                break;

            case Status::Initialized:
                if (m_Map.m_FileStore.IsLoaded() && !m_IsLoaded)
                {
                    lifecycle.BeginLoad();
                    if (!this->IsStable())
                    {
                        lifecycle.EndLoad();
                        break;
                    }

                    this->LoadResources();
                    lifecycle.EndLoad();

                    m_IsLoaded = true;
                }
                else
                {
                    std::this_thread::sleep_for(2ms);
                }
                break;

            case Status::Running:
            {
                std::uint64_t current{ lifecycle.WaitForTick(m_Last, m_Dropped) };
                m_Last = current;
                const std::uint64_t droppedBefore{ m_Dropped };

                // The game thread waits for this read, so the action tick of the bridge is the one of the state about to be copied.
                m_Transport.m_BridgeService.CaptureClock(m_Dropped == 0, m_BridgeClock);

                if (m_Dropped > 0)
                {
                    telemetry.RecordDroppedTicks(m_Dropped);
                    logs.Message("[AIThread] WARNING: Dropped {} ticks before tick {}.", m_Dropped, current);
                }
                m_Dropped = 0;

                lifecycle.BeginTick();
                if (!this->IsStable())
                {
                    lifecycle.EndRead(current);
                    lifecycle.EndTick();
                    break;
                }

                auto tickStart{ steady_clock::now() };
                m_StageNs.fill(0);
                m_Stage = 0;
                m_StageStart = tickStart;

                const bool isFirstAfterLoad{ std::exchange(m_IsFirstTickAfterLoad, false) };
                const auto snapshot{ this->ExecuteTick(current) };

                if (m_BridgeClock.IsDecision)
                {
                    m_Export.m_FlatStore.Push(FlatRequest{ .Snapshot = m_Export.m_TickStore.Acquire(), .Clock = m_BridgeClock });
                    m_Transport.m_BridgeService.MarkPushed(m_BridgeClock.ActionTick);
                }
                this->EndStage();

                if (!m_Observers.empty())
                {
                    const ObservedTick observed{ .Tick = m_Export.m_TickStore.Acquire(), .Snapshot = snapshot, .Generation = current,
                        .DroppedBefore = droppedBefore, .IsFirstAfterLoad = isFirstAfterLoad };
                    for (const auto& observer : m_Observers) observer(observed);
                }
                this->EndStage();

                auto tickEnd{ steady_clock::now() };
                lifecycle.EndTick();

                const auto sweepNs{ static_cast<std::uint64_t>(
                    std::chrono::duration_cast<nanoseconds>(tickEnd - tickStart).count()) };

                telemetry.RecordSweepTime(sweepNs);
                if (sweepNs >= static_cast<std::uint64_t>(k_SlowSweep.count())) this->ReportSlowSweep(sweepNs);

                break;
            }

            case Status::TearingDown:
                this->Reset();
                lifecycle.WaitForBlam();
                break;
            }
        }

        lifecycle.SetReaderActive(false);

        logs.Message("[AIThread] INFO: Stopped.");
    }

    auto AIThread::LoadResources() -> void
    {
        m_Map.m_MapBuilderService.LoadForMap();
        m_Resolved.m_DefinitionsBuilder.BuildForMap();
        m_Resolved.m_WorldBuilder.BuildForMap();
        m_Resolved.m_VitalityBuilder.BuildForMap();

        m_IsFirstTickAfterLoad = true;
    }

    auto AIThread::ExecuteTick(std::uint64_t generation) -> std::shared_ptr<const Snapshot>
    {
        const auto snapshot{ m_Capture.m_CaptureService.Capture() };

        // Last use of game memory: from here on everything works on the Snapshot, so the game thread runs the next tick.
        m_Base.m_LifecycleStore.EndRead(m_Last);
        this->EndStage();

        m_Export.m_PipelineService.Run(*snapshot, generation, m_EndStage);

        return snapshot;
    }

    auto AIThread::EndStage() -> void
    {
        auto now{ steady_clock::now() };

        if (m_Stage < m_StageNs.size())
        {
            m_StageNs[m_Stage] = static_cast<std::uint64_t>(std::chrono::duration_cast<nanoseconds>(now - m_StageStart).count());
            m_Stage++;
        }

        m_StageStart = now;
    }

    auto AIThread::ReportSlowSweep(std::uint64_t sweepNs) -> void
    {
        std::uint64_t stagesNs{ 0 };
        for (std::uint64_t ns : m_StageNs) stagesNs += ns;

        // Time not covered by a stage (the read handoff, telemetry).
        m_StageNs.back() = sweepNs > stagesNs ? sweepNs - stagesNs : 0;

        std::string stages{};
        for (std::size_t i{ 0 }; i < m_StageNs.size(); i++)
        {
            if (m_StageNs[i] < k_MinReportedStageNs) continue;
            stages += std::format("{}{} {:.1f} ms", stages.empty() ? "" : ", ", k_StageNames[i], m_StageNs[i] / 1e6);
        }

        m_Service.m_Log.Message("[AIThread] WARNING: Slow sweep of {:.1f} ms at tick {}: {}.", sweepNs / 1e6, m_Last, stages);
    }

    auto AIThread::IsStable() -> bool
    {
        auto& lifecycle{ m_Base.m_LifecycleStore };

        return lifecycle.IsRunning() &&
            lifecycle.GetStatus() != Status::TearingDown;
    }

    auto AIThread::Reset() -> void
    {
        m_Base.m_LifecycleStore.ResetTickGeneration();
        m_IsLoaded = false;
        m_Last = 0;
        m_Transport.m_BridgeService.ResetClock();
    }
}