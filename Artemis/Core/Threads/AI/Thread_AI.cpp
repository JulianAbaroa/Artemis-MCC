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

	uint64_t lastSeen = 0;

	while (lifecycle.IsRunning())
	{
		if (mapReader.IsLoaded() && !m_WasLoaded)
		{
			lastSeen = lifecycle.GetTickGen();

			// --- Layer 0: Sources ---
			m_Deps.System_TagGroupReader.LoadForMap();
			m_Deps.System_WorldBuilder.BuildForMap();
			m_Deps.System_StatsBuilder.BuildForMap();
			m_Deps.System_VitalityBuilder.BuildForMap();
			m_WasLoaded = true;

			lastSeen = lifecycle.GetTickGen();
		}

		if (!m_WasLoaded)
		{
			std::this_thread::sleep_for(50ms);
			continue;
		}

		uint64_t dropped = 0;
		uint64_t generation = lifecycle.WaitForTick(lastSeen, dropped);
		lastSeen = generation;

		if (!lifecycle.IsRunning()) break;

		if (lifecycle.IsTearingDown() ||
			lifecycle.GetEngineStatus() == EngineStatus::Destroyed ||
			!mapReader.IsLoaded())
		{
			m_WasLoaded = false;
			continue;
		}

		if (dropped > 0) telemetry.RecordDroppedTicks(dropped);

		lifecycle.BeginAISweep();
		auto sweepStart = SteadyClock::now();

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

		// Tick.
		m_Deps.System_Tick.Assemble(generation);

		auto sweepEnd = SteadyClock::now();
		lifecycle.EndAISweep();

		telemetry.RecordAISweep(
			std::chrono::duration_cast<NanoSeconds>(
				sweepEnd - sweepStart).count());
	}

	m_Deps.System_Logs.Log("[AIThread] INFO: Stopped.");
}