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

            // The game thread queues ticks only while this thread consumes them, and only once the resources of the map are loaded.
            this->SetConsuming(status == Status::Running && m_IsLoaded);

            switch (status)
            {
            case Status::Waiting:
            case Status::Destroyed:
                lifecycle.WaitForBlam();
                break;

            case Status::Initialized:
                if (m_Map.m_FileStore.IsLoaded() && !m_IsLoaded)
                {
                    const std::uint64_t loadEpoch{ lifecycle.GetTeardownEpoch() };

                    lifecycle.BeginLoad();
                    if (!this->IsStable())
                    {
                        lifecycle.EndLoad();
                        break;
                    }

                    this->LoadResources();
                    lifecycle.EndLoad();

                    // From here the game thread may queue ticks. The first one it queues is the first after the load (D207).
                    lifecycle.MarkLoaded(loadEpoch);
                    m_IsLoaded = true;
                }
                else
                {
                    std::this_thread::sleep_for(2ms);
                }
                break;

            case Status::Running:
            {
                std::optional<CapturedTick> tick{ m_TickQueue.WaitPop(k_PopTimeout) };
                if (tick.has_value()) this->ProcessTick(*tick);
                break;
            }

            case Status::TearingDown:
                this->Reset();
                lifecycle.WaitForBlam();
                break;
            }
        }

        this->SetConsuming(false);

        logs.Message("[AIThread] INFO: Stopped.");
    }

    auto AIThread::LoadResources() -> void
    {
        m_Map.m_MapBuilderService.LoadForMap();
        m_Resolved.m_DefinitionsBuilder.BuildForMap();
        m_Resolved.m_WorldBuilder.BuildForMap();
        m_Resolved.m_VitalityBuilder.BuildForMap();
    }

    auto AIThread::SetConsuming(bool value) -> void
    {
        if (m_IsConsuming == value) return;

        m_IsConsuming = value;
        m_TickQueue.SetOpen(value);
    }

    auto AIThread::ProcessTick(const CapturedTick& tick) -> void
    {
        auto& logs{ m_Service.m_Log };
        auto& telemetry{ m_Service.m_TelemetryStore };
        auto& lifecycle{ m_Base.m_LifecycleStore };

        m_Last = tick.Generation;
        m_TickEpoch = tick.Epoch;

        // The game thread waits for the tick in progress when the teardown starts, not for the ones still queued. The flag goes up before the
        // state is read, so a teardown that starts after the pop either sees the flag or is seen here (LifecycleStore::BeginTick).
        lifecycle.BeginTick();
        if (!this->IsStable() || tick.Epoch != lifecycle.GetTeardownEpoch())
        {
            this->DiscardTick(tick.Generation);
            lifecycle.EndTick();
            return;
        }

        if (tick.DroppedBefore > 0)
        {
            logs.Message("[AIThread] WARNING: Dropped {} ticks before tick {}.", tick.DroppedBefore, tick.Generation);
        }

        const auto tickStart{ steady_clock::now() };
        m_StageNs.fill(0);
        m_Stage = 0;
        m_StageStart = tickStart;

        // Sparse mode (Q101): the game thread chose a light capture for a tick that is not a decision.
        if (tick.IsLight)
        {
            if (!this->ExecuteLightTick(tick))
            {
                this->DiscardTick(tick.Generation);
                lifecycle.EndTick();
                return;
            }

            const auto lightEnd{ steady_clock::now() };
            lifecycle.EndTick();
            telemetry.RecordLightTick(static_cast<std::uint64_t>(
                std::chrono::duration_cast<nanoseconds>(lightEnd - tickStart).count()));
            return;
        }

        // The engine can start tearing down while the tick runs: the Cleanup empties the stores it writes, and the game thread waits
        // for this tick at most 1 s (C.9). The state is read again here, before anything is published, not only when the tick began.
        if (!this->ExecuteTick(tick) || !this->IsCurrent())
        {
            this->DiscardTick(tick.Generation);
            lifecycle.EndTick();
            return;
        }

        if (tick.Clock.IsDecision)
        {
            m_Export.m_FlatStore.Push(FlatRequest{ .Snapshot = m_Export.m_TickStore.Acquire(), .Clock = tick.Clock });
        }
        this->EndStage();

        if (!m_Observers.empty())
        {
            const ObservedTick observed{ .Tick = m_Export.m_TickStore.Acquire(), .Snapshot = tick.Snapshot, .Generation = tick.Generation,
                .DroppedBefore = tick.DroppedBefore, .IsFirstAfterLoad = tick.IsFirstAfterLoad };
            for (const auto& observer : m_Observers) observer(observed);
        }
        this->EndStage();

        const auto tickEnd{ steady_clock::now() };
        lifecycle.EndTick();

        const auto sweepNs{ static_cast<std::uint64_t>(
            std::chrono::duration_cast<nanoseconds>(tickEnd - tickStart).count()) };

        // Time not covered by a stage (the telemetry) goes to the last one.
        m_StageNs.back() = OtherStageNs(sweepNs, std::span<const std::uint64_t>{ m_StageNs }.first(m_StageNs.size() - 1));
        telemetry.RecordSweep(sweepNs, m_StageNs);
        if (sweepNs >= static_cast<std::uint64_t>(k_SlowSweep.count())) this->ReportSlowSweep(sweepNs);
    }

    auto AIThread::ExecuteTick(const CapturedTick& tick) -> bool
    {
        // A teardown that began after the copy: the pipeline would only write stores the Cleanup is emptying.
        if (!this->IsCurrent()) return false;

        m_Export.m_PipelineService.Run(*tick.Snapshot, tick.Generation, m_EndStage, m_CanPublish);

        // The pipeline stops between stages when the teardown starts: what it left is not a tick to publish.
        return this->IsCurrent();
    }

    auto AIThread::ExecuteLightTick(const CapturedTick& tick) -> bool
    {
        if (!this->IsCurrent()) return false;

        m_Export.m_PipelineService.RunLight(*tick.Snapshot);

        return this->IsCurrent();
    }

    auto AIThread::DiscardTick(std::uint64_t generation) -> void
    {
        m_Base.m_LifecycleStore.AddDiscardedTick();
        m_Service.m_Log.Message("[AIThread] INFO: Tick {} discarded: the engine is tearing down.", generation);
    }

    auto AIThread::DiscardPending() -> void
    {
        const std::size_t discarded{ m_TickQueue.Clear() };
        if (discarded == 0) return;

        for (std::size_t i{ 0 }; i < discarded; ++i) m_Base.m_LifecycleStore.AddDiscardedTick();
        m_Service.m_Log.Message("[AIThread] INFO: {} queued ticks discarded: the engine is tearing down.", discarded);
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

    auto AIThread::IsCurrent() const -> bool
    {
        return m_Base.m_LifecycleStore.CanPublish() && m_TickEpoch == m_Base.m_LifecycleStore.GetTeardownEpoch();
    }

    auto AIThread::IsStable() -> bool
    {
        auto& lifecycle{ m_Base.m_LifecycleStore };

        return lifecycle.IsRunning() &&
            lifecycle.GetStatus() != Status::TearingDown;
    }

    auto AIThread::Reset() -> void
    {
        this->SetConsuming(false);
        this->DiscardPending();
        m_IsLoaded = false;
        m_Last = 0;
    }
}