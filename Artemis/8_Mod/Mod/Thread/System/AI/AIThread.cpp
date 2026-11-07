module Mod.Thread.System;
import :AI;

import Services.Telemetry.Type;
import Services.Telemetry.System;
import Engine.Base.Lifecycle.Type;
import Bridge.Codec.Flat.Type;
import std;

using namespace std::chrono_literals;

namespace
{
    using Engine::Base::Lifecycle::Type::Status;
    using Bridge::Codec::Flat::Type::FlatRequest;
    using Services::Telemetry::System::OtherStageNs;
    using Services::Telemetry::Type::k_MaxStages;

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

        static_assert(k_StageNames.size() <= k_MaxStages, "The telemetry store keeps the time of up to k_MaxStages stages.");

        logs.Message("[AIThread] INFO: Started.");
        telemetry.SetStageNames(k_StageNames);

        while (lifecycle.IsRunning())
        {
            // A teardown happened since the last reset: the state of the previous map goes, even if this thread never saw TearingDown
            // (a tick that outlasted the whole teardown sees Destroyed, and one that overslept can wake in the next Initialized).
            if (const std::uint64_t epoch{ lifecycle.GetTeardownEpoch() }; epoch != m_ResetEpoch)
            {
                this->Reset();
                m_ResetEpoch = epoch;
            }

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

                // Sparse mode (Q101): a tick that is not a decision only does the light pass. The first tick after a load and the first
                // one after the mode changed to sparse run the whole pipeline, so the tick of a decision always has the state it needs.
                const bool wasSparse{ std::exchange(m_IsSparse, m_BridgeClock.IsSparse) };
                if (m_BridgeClock.IsSparse && !m_BridgeClock.IsDecision && wasSparse && !isFirstAfterLoad)
                {
                    if (!this->ExecuteLightTick())
                    {
                        this->DiscardTick(current);
                        lifecycle.EndTick();
                        break;
                    }

                    const auto lightEnd{ steady_clock::now() };
                    lifecycle.EndTick();
                    telemetry.RecordLightTick(static_cast<std::uint64_t>(
                        std::chrono::duration_cast<nanoseconds>(lightEnd - tickStart).count()));
                    break;
                }

                const auto snapshot{ this->ExecuteTick(current) };

                // The engine can start tearing down while the tick runs: the Cleanup empties the stores it writes and does not wait
                // for it past its timeout. The state is read again here, before anything is published, not only when the tick began.
                if (snapshot == nullptr || !lifecycle.CanPublish())
                {
                    this->DiscardTick(current);
                    lifecycle.EndTick();
                    break;
                }

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

                // Time not covered by a stage (the read handoff, telemetry) goes to the last one.
                m_StageNs.back() = OtherStageNs(sweepNs, std::span<const std::uint64_t>{ m_StageNs }.first(m_StageNs.size() - 1));
                telemetry.RecordSweep(sweepNs, m_StageNs);
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

        // A teardown that began during the read: the pipeline would only write stores the Cleanup is emptying.
        if (!m_Base.m_LifecycleStore.CanPublish()) return nullptr;

        m_Export.m_PipelineService.Run(*snapshot, generation, m_EndStage, m_CanPublish);

        // The pipeline stops between stages when the teardown starts: what it left is not a tick to publish.
        if (!m_Base.m_LifecycleStore.CanPublish()) return nullptr;

        return snapshot;
    }

    auto AIThread::ExecuteLightTick() -> bool
    {
        const auto snapshot{ m_Capture.m_CaptureService.CaptureLight() };

        // Last use of game memory, as in ExecuteTick.
        m_Base.m_LifecycleStore.EndRead(m_Last);

        if (snapshot == nullptr || !m_Base.m_LifecycleStore.CanPublish()) return false;

        m_Export.m_PipelineService.RunLight(*snapshot);

        return m_Base.m_LifecycleStore.CanPublish();
    }

    auto AIThread::DiscardTick(std::uint64_t generation) -> void
    {
        m_Base.m_LifecycleStore.AddDiscardedTick();
        m_Service.m_Log.Message("[AIThread] INFO: Tick {} discarded: the engine is tearing down.", generation);
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
        m_IsSparse = false;
        m_Last = 0;
        m_Transport.m_BridgeService.ResetClock();
    }
}