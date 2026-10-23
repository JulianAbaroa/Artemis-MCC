module Runtime.Thread;
import :AI;

import Platform.Lifecycle.Type;
import Export.Flat.Type;
import std;

using namespace std::chrono_literals;

namespace
{
    using Platform::Lifecycle::Type::Status;
    using Export::Flat::Type::FlatRequest;

    using std::chrono::steady_clock;
    using std::chrono::nanoseconds;

    // Sweeps at or above this are logged with the time of each stage.
    constexpr nanoseconds k_SlowSweep{ 50'000'000 };
    // Stages below this are left out of the slow sweep log.
    constexpr std::uint64_t k_MinReportedStageNs{ 100'000 };
}

namespace Runtime::Thread
{
    auto AIThread::Run() -> void
    {
        auto& logs{ m_Service.m_Log };
        auto& telemetry{ m_Service.m_TelemetryStore };
        auto& lifecycle{ m_Platform.m_LifecycleStore };

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
                m_Platform.m_BridgeService.CaptureClock(m_Dropped == 0, m_BridgeClock);

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

                m_Platform.m_InputService.AdvanceInputTick();
                this->EndStage();

                this->ExecuteTick();

                m_Export.m_TickService.Assemble(current);
                if (m_BridgeClock.IsDecision)
                {
                    m_Export.m_FlatStore.Push(FlatRequest{ .Snapshot = m_Export.m_TickStore.Acquire(), .Clock = m_BridgeClock });
                    m_Platform.m_BridgeService.MarkPushed(m_BridgeClock.ActionTick);
                }
                this->EndStage();

                m_TickTraceUIService.Record(current);
                if (m_TickDumpUIService.IsRecording()) m_TickDumpUIService.Push(m_Export.m_TickStore.Acquire(), droppedBefore);

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
    }

    auto AIThread::ExecuteTick() -> void
    {
        m_Datum.m_DatumService.UpdateObjectTable();
        this->EndStage();

        m_Tables.m_ObjectService.UpdateObjectTable();
        this->EndStage();
        m_Tables.m_PlayerService.UpdatePlayerTable();
        this->EndStage();
        m_Tables.m_InteractionService.UpdateInteractionTable();
        this->EndStage();
        m_Tables.m_GameService.Update();
        this->EndStage();

        // The engine clock of the trace is read here too, so it matches the copied tables.
        m_TickTraceUIService.CaptureClock();

        // Last read of game memory: from here on the services work on their own copies, so the game thread can run the next tick.
        m_Platform.m_LifecycleStore.EndRead(m_Last);

        m_Relations.m_ClassifierService.UpdateClassification();
        this->EndStage();
        m_Relations.m_ObjectGraphService.UpdateGraph();
        this->EndStage();

        m_Environment.m_VitalityDetailsService.Update();
        this->EndStage();
        m_Environment.m_ZoneDetailsService.Update();
        this->EndStage();
        m_Environment.m_TeleporterDetailsService.Update();
        this->EndStage();
        m_Environment.m_LiftDetailsService.Update();
        this->EndStage();
        m_Environment.m_BarrierDetailsService.Update();
        this->EndStage();
        m_Environment.m_EntitiesService.Update();
        this->EndStage();
        m_Environment.m_PlayersService.Update();
        this->EndStage();
        m_Environment.m_CollidableService.Update(m_ViewerCameraStore.IsActive());
        this->EndStage();
        m_Environment.m_AimService.Update();
        this->EndStage();
        m_Environment.m_EventsService.Update();
        this->EndStage();
        m_Environment.m_MatchService.Update();
        this->EndStage();

        m_Egocentric.m_SelfService.Update();
        this->EndStage();
        m_Egocentric.m_AffordanceService.Update();
        this->EndStage();
        m_Egocentric.m_RaycastService.Update();
        this->EndStage();

        // The export step is not here. Run() assembles it because it needs the tick generation.
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
        auto& lifecycle{ m_Platform.m_LifecycleStore };

        return lifecycle.IsRunning() &&
            lifecycle.GetStatus() != Status::TearingDown;
    }

    auto AIThread::Reset() -> void
    {
        m_Platform.m_LifecycleStore.ResetTickGeneration();
        m_IsLoaded = false;
        m_Last = 0;
        m_Platform.m_BridgeService.ResetClock();
    }
}