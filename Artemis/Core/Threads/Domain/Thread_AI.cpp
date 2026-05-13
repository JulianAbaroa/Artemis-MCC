#include "pch.h"

// Header.
#include "Thread_AI.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Player/State_PlayerTable.h"
#include "Core/States/Domain/Navigation/State_Navigation.h"

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

// --- Systems ---
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"

#include "Core/Systems/Domain/Object/System_ObjectTable.h"
#include "Core/Systems/Domain/Player/System_PlayerTable.h"
#include "Core/Systems/Domain/Interaction/System_InteractionTable.h"
#include "Core/Systems/Domain/Classification/System_ObjectClassifier.h"
#include "Core/Systems/Domain/Graph/System_ObjectGraph.h"
#include "Core/Systems/Domain/Graph/System_PlayerGraph.h"
#include "Core/Systems/Domain/Navigation/System_Navigation.h"
#include "Core/Systems/Domain/Environment/System_Environment.h"
#include "Core/Systems/Domain/Interactable/System_Interactable.h"

#include "Core/Systems/Interface/System_Debug.h"

#include <chrono>

using namespace std::chrono_literals;

void Thread_AI::Run()
{
	g_pSystem->Debug->Log("[AIThread] INFO: Started.");

	while (g_pState->Infrastructure->Lifecycle->IsRunning())
	{
		if (g_pState->Domain->Map->IsLoaded())
		{
			// Update tables.
			g_pSystem->Domain->ObjectTable->UpdateObjectTable();
			g_pSystem->Domain->PlayerTable->UpdatePlayerTable();
			g_pSystem->Domain->InteractionTable->UpdateInteractionTable();

			// Classify objects.
			g_pSystem->Domain->ObjectClassifier->UpdateClassification();

			// Update graphs.
			g_pSystem->Domain->ObjectGraph->UpdateGraph();
			g_pSystem->Domain->PlayerGraph->UpdateGraph();

			if (!g_pState->Domain->Navigation->HasSbspGeometry()) continue;

			// TODO: Domains: Agents, Self.

			g_pSystem->Domain->Navigation->UpdateNavigation();
			g_pSystem->Domain->Environment->UpdateEnvironment();
			g_pSystem->Domain->Interactable->UpdateInteractables();
		}

		std::this_thread::sleep_for(16ms);
	}

	g_pSystem->Debug->Log("[AIThread] INFO: Stopped.");
}