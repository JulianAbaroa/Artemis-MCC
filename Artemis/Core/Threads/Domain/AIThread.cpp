#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Map/MapState.h"
#include "Core/States/Domain/Navigation/NavigationState.h"
#include "Core/States/Domain/Tables/PlayerTableState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Tables/ObjectTableSystem.h"
#include "Core/Systems/Domain/Tables/PlayerTableSystem.h"
#include "Core/Systems/Domain/Tables/InteractionTableSystem.h"
#include "Core/Systems/Domain/Graph/ObjectGraphSystem.h"
#include "Core/Systems/Domain/Interactable/InteractableSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/Domain/AIThread.h"
#include <chrono>

using namespace std::chrono_literals;

void AIThread::Run()
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

			// Update the father-child-sibling object graph.
			g_pSystem->Domain->ObjectGraph->UpdateGraph();

			// Next systems are dependent on the geometry of the map.
			if (!g_pState->Domain->Navigation->HasGeometry()) return;

			// Temporal way to get the AI-self.
			uint32_t selfPlayer = 
				g_pState->Domain->PlayerTable->GetPlayerHandleByName(
					"PlaceHolder021");

			// TODO: Domains:
			// - UpdateNavigation.
			// - UpdateInteractable.
			// - UpdateAgents.
			// - UpdateSelf.

			//g_pSystem->Domain->Interactable->UpdateInteractables(selfPlayer);
		}

		std::this_thread::sleep_for(16ms);
	}

	g_pSystem->Debug->Log("[AIThread] INFO: Stopped.");
}