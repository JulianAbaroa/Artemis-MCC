#include "pch.h"

// Header.
#include "System_ObjectGraph.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

// Map.
#include "Core/States/Domain/Map/Bipd/State_MapBipd.h"
#include "Core/States/Domain/Map/Bloc/State_MapBloc.h"
#include "Core/States/Domain/Map/Coll/State_MapColl.h"
#include "Core/States/Domain/Map/Eqip/State_MapEqip.h"
#include "Core/States/Domain/Map/Jmad/State_MapJmad.h"
#include "Core/States/Domain/Map/Mach/State_MapMach.h"
#include "Core/States/Domain/Map/Mode/State_MapMode.h"
#include "Core/States/Domain/Map/Phmo/State_MapPhmo.h"
#include "Core/States/Domain/Map/Proj/State_MapProj.h"
#include "Core/States/Domain/Map/Scen/State_MapScen.h"
#include "Core/States/Domain/Map/Scnr/State_MapScnr.h"
#include "Core/States/Domain/Map/Vehi/State_MapVehi.h"
#include "Core/States/Domain/Map/Weap/State_MapWeap.h"
#include "Core/States/Domain/Map/Ctrl/State_MapCtrl.h"

// Object.
#include "Core/States/Domain/Object/State_ObjectTable.h"

// Graph.
#include "Core/States/Domain/Graph/State_ObjectGraph.h"

// Systems.
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

// Called from 'Thread_AI::Run', its responsible for generating & updating
// the father-child-sibling relationship between objects. These relationships
// are called ObjectNodes and PlayerTrees.
void System_ObjectGraph::UpdateGraph()
{
	std::unordered_map<uint32_t, ObjectNode> nodes;
	this->BuildNodes(nodes);
	g_pState->Domain->ObjectGraph->SetNodes(std::move(nodes));
}

void System_ObjectGraph::Cleanup()
{
	g_pState->Domain->ObjectGraph->Cleanup();
	g_pSystem->Debug->Log("[System_ObjectGraph] INFO: Cleanup completed.");
}

void System_ObjectGraph::BuildNodes(std::unordered_map<uint32_t, ObjectNode>& nodes)
{
	// Get the entire Artemis object table.
	const auto& objectTable = g_pState->Domain->ObjectTable->GetObjectTable();

	for (const auto& [handle, object] : objectTable)
	{
		ObjectNode node{};
		node.Handle = handle;
		node.ParentHandle = object.ParentHandle;
		
		uint32_t childHandle = object.ChildHandle;
		while (childHandle != 0xFFFFFFFF)
		{
			auto it = objectTable.find(childHandle);
			if (it == objectTable.end()) break;

			node.ChildrenHandles.push_back(childHandle);
			childHandle = it->second.NextSiblingHandle;
		}

		nodes.emplace(handle, std::move(node));
	}
}