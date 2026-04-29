#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Navigation/State_Navigation.h"
#include "Core/States/Domain/Tables/State_PlayerTable.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Tables/System_ObjectTable.h"
#include "Core/Systems/Domain/Tables/System_PlayerTable.h"
#include "Core/Systems/Domain/Tables/System_InteractionTable.h"
#include "Core/Systems/Domain/Graph/System_ObjectGraph.h"
#include "Core/Systems/Domain/Interactable/System_Interactable.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "Core/Threads/Domain/Thread_AI.h"
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

			// Update father-child-sibling object graph.
			g_pSystem->Domain->ObjectGraph->UpdateGraph();

			// Next systems are dependent on the geometry of the map.
			if (!g_pState->Domain->Navigation->HasGeometry()) continue;

			// Temporal way to get the AI-self.
			uint32_t selfPlayer = 
				g_pState->Domain->PlayerTable->GetPlayerHandleByName(
					"PlaceHolder021");

			// TODO: Domains:
			// - UpdateNavigation.
			// - UpdateInteractable.
			// - UpdateAgents.
			// - UpdateSelf.

			g_pSystem->Domain->Interactable->UpdateInteractables(selfPlayer);
		}

		std::this_thread::sleep_for(16ms);
	}

	g_pSystem->Debug->Log("[AIThread] INFO: Stopped.");
}