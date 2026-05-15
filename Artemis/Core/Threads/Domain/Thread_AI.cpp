#include "pch.h"

// Header.
#include "Thread_AI.h"

// --- States ---

#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Player/State_PlayerTable.h"
#include "Core/States/Domain/Navigation/State_Navigation.h"

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

// --- Systems ---

#include "Core/Systems/Domain/Object/System_ObjectTable.h"
#include "Core/Systems/Domain/Player/System_PlayerTable.h"
#include "Core/Systems/Domain/Interaction/System_InteractionTable.h"
#include "Core/Systems/Domain/Classification/System_ObjectClassifier.h"
#include "Core/Systems/Domain/Graph/System_ObjectGraph.h"
#include "Core/Systems/Domain/Graph/System_PlayerGraph.h"
#include "Core/Systems/Domain/Navigation/System_Navigation.h"
#include "Core/Systems/Domain/Environment/System_Environment.h"
#include "Core/Systems/Domain/Interactable/System_Interactable.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

#include <chrono>

using namespace std::chrono_literals;

void Thread_AI::Run()
{
	m_Deps.System_Debug.Log("[AIThread] INFO: Started.");

	while (m_Deps.State_Lifecycle.IsRunning())
	{
		if (m_Deps.State_Map.IsLoaded())
		{
			// Update tables.
			m_Deps.System_ObjectTable.UpdateObjectTable();
			m_Deps.System_PlayerTable.UpdatePlayerTable();
			m_Deps.System_InteractionTable.UpdateInteractionTable();

			// Classify objects.
			m_Deps.System_ObjectClassifier.UpdateClassification();

			// Update graphs.
			m_Deps.System_ObjectGraph.UpdateGraph();
			m_Deps.System_PlayerGraph.UpdateGraph();

			if (!m_Deps.State_Navigation.HasSbspGeometry()) continue;

			// TODO: Domains: Agents, Self, Observable.

			// Update domains.
			m_Deps.System_Navigation.UpdateNavigation();
			m_Deps.System_Environment.UpdateEnvironment();
			m_Deps.System_Interactable.UpdateInteractables();
		}

		std::this_thread::sleep_for(16ms);
	}

	m_Deps.System_Debug.Log("[AIThread] INFO: Stopped.");
}