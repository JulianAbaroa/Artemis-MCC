module Runtime.Thread;
import :AI;

import Platform.Lifecycle.Type;
import std;

using namespace std::chrono_literals;

namespace
{
    using Status = Platform::Lifecycle::Type::Status;

    using std::chrono::steady_clock;
    using std::chrono::nanoseconds;
}

namespace Runtime::Thread
{
    auto AIThread::Run() -> void
    {
        auto& logs{ m_Service.m_LogsService };
        auto& telemetry{ m_Service.m_TelemetryStore };
        auto& lifecycle{ m_Platform.m_LifecycleStore };

        logs.Message("[AIThread] INFO: Started.");

        while (lifecycle.IsRunning())
        {
            switch (lifecycle.GetStatus())
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

                if (m_Dropped > 0) telemetry.RecordDroppedTicks(m_Dropped);
                m_Dropped = 0;

                lifecycle.BeginTick();
                if (!this->IsStable())
                {
                    lifecycle.EndTick();
                    break;
                }

                auto tickStart{ steady_clock::now() };

                m_Platform.m_InputService.AdvanceInputTick();

                this->ExecuteTick();
                m_Export.m_TickService.Assemble(current);

                auto tickEnd{ steady_clock::now() };
                lifecycle.EndTick();

                telemetry.RecordSweepTime(static_cast<std::uint64_t>(
                    std::chrono::duration_cast<nanoseconds>(tickEnd - tickStart).count())
                );

                break;
            }

            case Status::TearingDown:
                this->Reset();
                lifecycle.WaitForBlam();
                break;
            }
        }

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
        m_Template.m_TemplateService.UpdateObjectTable();

        m_Tables.m_ObjectService.UpdateObjectTable();
        m_Tables.m_PlayerService.UpdatePlayerTable();
        m_Tables.m_InteractionService.UpdateInteractionTable();

        m_Relations.m_ClassifierService.UpdateClassification();
        m_Relations.m_ObjectGraphService.UpdateGraph();
        m_Relations.m_PlayerGraphService.UpdateGraph();

        m_Environment.m_CollidableService.Update(m_ViewerCameraStore.IsActive());
        m_Environment.m_FixturesService.Update();
        m_Environment.m_HealthService.Update();
        m_Environment.m_AimService.Update();

        m_Egocentric.m_SelfService.Update();
        m_Egocentric.m_AffordanceService.Update();
        m_Egocentric.m_RaycastService.Update();

        // The export step is not here. Run() assembles it because it needs the tick generation.
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
    }
}