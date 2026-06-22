#include "pch.h"

#include "Thread_AI.h"

#include "Core/States/Sources/MapReader/State_MapReader.h"
#include "Core/States/Sources/Static/World/State_WorldBuilder.h"
#include "Core/States/Other/Telemetry/State_Telemetry.h"
#include "Core/States/Other/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/Sources/MapReader/TagGroup/System_TagGroupReader.h"
#include "Core/Systems/Sources/Tables/Object/System_ObjectTable.h"
#include "Core/Systems/Sources/Tables/Player/System_PlayerTable.h"
#include "Core/Systems/Sources/Tables/Interaction/System_InteractionTable.h"
#include "Core/Systems/Sources/Static/World/System_WorldBuilder.h"
#include "Core/Systems/Sources/Static/Stats/System_StatsBuilder.h"
#include "Core/Systems/Sources/Static/Vitality/System_VitalityBuilder.h"
#include "Core/Systems/Structure/Classifier/System_Classifier.h"
#include "Core/Systems/Structure/Graph/Object/System_ObjectGraph.h"
#include "Core/Systems/Structure/Graph/Player/System_PlayerGraph.h"
#include "Core/Systems/Environment/Collidables/System_Collidables.h"
#include "Core/Systems/Environment/Vitality/System_Vitality.h"
#include "Core/Systems/Environment/Fixtures/System_Fixtures.h"
#include "Core/Systems/Egocentric/Affordances/System_Affordances.h"
#include "Core/Systems/Egocentric/Self/System_Self.h"
#include "Core/Systems/Tick/System_Tick.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

using namespace std::chrono_literals;

void Thread_AI::Run()
{
	m_Deps.System_Logs.Log("[AIThread] INFO: Started.");

	auto& lifecycle = m_Deps.State_Lifecycle;
	auto& telemetry = m_Deps.State_Telemetry;
	auto& mapReader = m_Deps.State_MapReader;

	while (lifecycle.IsRunning())
	{
		auto status = lifecycle.GetStatus();

		switch (status)
		{
		case Status::Waiting:
			break;

		case Status::Initialized:
			if (mapReader.IsLoaded() && !m_WasLoaded)
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
			break;

		case Status::Running:
		{
			uint64_t current = lifecycle.WaitForTick(m_Last, m_Dropped);
			m_Last = current;

			if (m_Dropped > 0) telemetry.RecordDroppedTicks(m_Dropped);
			m_Dropped = 0;

			lifecycle.BeginTick();
			if (!this->IsStable())
			{
				lifecycle.EndTick();
				break;
			}

			std::chrono::time_point tickStart = SteadyClock::now();

			this->ExecuteTick();

			m_Deps.System_Tick.Assemble(current);

			std::chrono::time_point tickEnd = SteadyClock::now();
			lifecycle.EndTick();

			telemetry.RecordTickTime(
				std::chrono::duration_cast<NanoSeconds>(
					tickEnd - tickStart).count());
			break;
		}

		case Status::TearingDown:
			this->Reset();
			lifecycle.WaitForBlam();
			break;

		case Status::Destroyed:
			break;
		}
	}

	m_Deps.System_Logs.Log("[AIThread] INFO: Stopped.");
}

void Thread_AI::LoadResources()
{
	auto& lifecycle = m_Deps.State_Lifecycle;

	// --- Layer 0: Sources ---
	m_Deps.System_TagGroupReader.LoadForMap();
	m_Deps.System_WorldBuilder.BuildForMap();
	m_Deps.System_StatsBuilder.BuildForMap();
	m_Deps.System_VitalityBuilder.BuildForMap();
}

void Thread_AI::ExecuteTick()
{
	// --- Layer 0: Sources ---
	m_Deps.System_ObjectTable.UpdateObjectTable();
	m_Deps.System_PlayerTable.UpdatePlayerTable();
	m_Deps.System_InteractionTable.UpdateInteractionTable();

	// --- Layer 1: Structure ---
	m_Deps.System_Classifier.UpdateClassification();
	m_Deps.System_ObjectGraph.UpdateGraph();
	m_Deps.System_PlayerGraph.UpdateGraph();

	// --- Layer 2: Environment ---
	m_Deps.System_Collidables.Update();
	m_Deps.System_Fixtures.Update();
	m_Deps.System_Vitality.Update();

	// --- Layer 3: Egocentric ---
	m_Deps.System_Self.Update();
	m_Deps.System_Affordances.Update();
}

bool Thread_AI::IsStable()
{
	auto& lifecycle = m_Deps.State_Lifecycle;

	if (!lifecycle.IsRunning() ||
		lifecycle.GetStatus() == Status::TearingDown)
	{
		return false;
	}

	return true;
}

void Thread_AI::Reset()
{
	auto& lifecycle = m_Deps.State_Lifecycle;

	lifecycle.ResetTickGeneration();
	m_WasLoaded = false;
	m_Last = 0;
}