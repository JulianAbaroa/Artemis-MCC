module Runtime.Thread;
import :AI;

import Platform.Lifecycle.Type;
import std;

using namespace std::chrono_literals;

namespace
{
	using Status = Platform::Lifecycle::Type::Status;
	using SteadyClock = std::chrono::steady_clock;
	using NanoSeconds = std::chrono::nanoseconds;
}

namespace Runtime::Thread
{
	auto AIThread::Run() -> void
	{
		auto& logs = m_Service.m_LogsService;
		auto& telemetry = m_Service.m_TelemetryStore;
		auto& lifecycle = m_Platform.m_LifecycleStore;

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
				if (m_Map.m_FileStore.IsLoaded() && !m_WasLoaded)
				{
					lifecycle.BeginLoad();
					if (!this->IsStable())
					{
						lifecycle.EndLoad();
						break;
					}

					this->LoadResources();
					lifecycle.EndLoad();

					m_WasLoaded = true;
				}
				else
				{
					std::this_thread::sleep_for(2ms);
				}
				break;

			case Status::Running:
			{
				std::uint64_t current = lifecycle.WaitForTick(m_Last, m_Dropped);
				m_Last = current;

				if (m_Dropped > 0) telemetry.RecordDroppedTicks(m_Dropped);
				m_Dropped = 0;

				lifecycle.BeginTick();
				if (!this->IsStable())
				{
					lifecycle.EndTick();
					break;
				}

				auto tickStart = SteadyClock::now();

				this->ExecuteTick();
				m_Export.m_TickService.Assemble(current);

				auto tickEnd = SteadyClock::now();
				lifecycle.EndTick();

				telemetry.RecordTickTime(static_cast<std::uint64_t>(
					std::chrono::duration_cast<NanoSeconds>(tickEnd - tickStart).count())
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
		// --- Map / Resolved ---
		m_Map.m_MapBuilderService.LoadForMap();
		m_Resolved.m_WorldBuilder.BuildForMap();
		m_Resolved.m_StatsBuilder.BuildForMap();
		m_Resolved.m_VitalityBuilder.BuildForMap();
	}

	auto AIThread::ExecuteTick() -> void
	{
		// --- Tables ---
		m_Tables.m_ObjectService.UpdateObjectTable();
		m_Tables.m_PlayerService.UpdatePlayerTable();
		m_Tables.m_InteractionService.UpdateInteractionTable();

		// --- Relations ---
		m_Relations.m_ClassifierService.UpdateClassification();
		m_Relations.m_ObjectGraphService.UpdateGraph();
		m_Relations.m_PlayerGraphService.UpdateGraph();

		// --- Environment ---
		m_Environment.m_CollidableService.Update(m_ViewerCameraStore.IsActive());
		m_Environment.m_FixturesService.Update();
		m_Environment.m_HealthService.Update();

		// --- Egocentric ---
		m_Egocentric.m_SelfService.Update();
		m_Egocentric.m_AffordanceService.Update();
		m_Egocentric.m_RaycastService.Update();

		// --- Export --- (Assemble(current) is called by Run(), it needs the generation)
	}

	auto AIThread::IsStable() -> bool
	{
		auto& lifecycle = m_Platform.m_LifecycleStore;

		return lifecycle.IsRunning() &&
			lifecycle.GetStatus() != Status::TearingDown;
	}

	auto AIThread::Reset() -> void
	{
		m_Platform.m_LifecycleStore.ResetTickGeneration();
		m_WasLoaded = false;
		m_Last = 0;
	}
}