#include "pch.h"

// Header.
#include "System_PlayerGraph.h"

// States.
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

// Player.
#include "Core/States/Domain/Player/State_PlayerTable.h"

// Object.
#include "Core/States/Domain/Object/State_ObjectTable.h"

// Classification.
#include "Core/States/Domain/Classification/State_Classification.h"

// Graph.
#include "Core/States/Domain/Graph/State_ObjectGraph.h"
#include "Core/States/Domain/Graph/State_PlayerGraph.h"

// Systems.
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

// Called from 'Thread_AI::Run'. Responsible of updating the player trees.
void System_PlayerGraph::UpdateGraph()
{
	std::vector<PlayerTree> trees;
	this->BuildPlayerTrees(trees);
	g_pState->Domain->PlayerGraph->SetTrees(std::move(trees));
}

void System_PlayerGraph::Cleanup()
{
	g_pState->Domain->PlayerGraph->Cleanup();
	g_pSystem->Debug->Log("[PlayerGraph] INFO: Cleanup completed.");
}

void System_PlayerGraph::BuildPlayerTrees(std::vector<PlayerTree>& trees)
{
	const auto& playerTable = g_pState->Domain->PlayerTable->GetPlayerTable();
	const auto& nodes = g_pState->Domain->ObjectGraph->GetNodes();

	for (const auto& [handle, player] : playerTable)
	{
		if (!player.Handle) continue;

		PlayerTree tree{};
		tree.Handle = handle;

		// Biped.
		if (player.AliveBipedHandle != 0xFFFFFFFF)
		{
			if (nodes.contains(player.AliveBipedHandle))
			{
				tree.BipedHandle = player.AliveBipedHandle;
			}
		}

		// Weapons.
		if (player.PrimaryWeaponHandle != 0xFFFFFFFF)
		{
			if (nodes.contains(player.PrimaryWeaponHandle))
			{
				tree.PrimaryWeaponHandle = player.PrimaryWeaponHandle;
			}
		}
		if (player.SecondaryWeaponHandle != 0xFFFFFFFF)
		{
			if (nodes.contains(player.SecondaryWeaponHandle))
			{
				tree.SecondaryWeaponHandle = player.SecondaryWeaponHandle;
			}
		}

		// Objective.
		if (player.ObjectiveHandle != 0xFFFFFFFF)
		{
			if (nodes.contains(player.ObjectiveHandle))
			{
				tree.ObjectiveHandle = player.ObjectiveHandle;
			}
		}

		// Armor ability.

		// We get the classified objects.
		const auto& classifiedObjects = g_pState->Domain->Classification->GetObjects();
		for (const auto& classified : classifiedObjects)
		{
			// We skip everything that's not a EquipmentEquiped.
			if (classified.Role != ObjectRole::ArmorAbilityEquiped) continue;

			// We search for the handle of this classified 
			// object inside our object node graph.
			auto it = nodes.find(classified.Handle);
			if (it == nodes.end()) continue;

			// If this armor ability parent handle is the same as the biped handle 
			// of this player, it means this armor ability is owned by this player.
			if (it->second.ParentHandle == tree.BipedHandle)
			{
				tree.AbilityHandle = classified.Handle;
				break;
			}
		}

		// Vehicle.

		if (tree.BipedHandle != 0xFFFFFFFF)
		{
			this->BuildVehicle(tree, nodes);
		}

		trees.push_back(std::move(tree));
	}
}

void System_PlayerGraph::BuildVehicle(PlayerTree& tree,
	const std::unordered_map<uint32_t, ObjectNode>& nodes)
{
	auto bipedIt = nodes.find(tree.BipedHandle);
	if (bipedIt == nodes.end()) return;

	// We get the parent handle of this player's biped.
	uint32_t parentHandle = bipedIt->second.ParentHandle;
	if (parentHandle == 0xFFFFFFFF) return;

	// We get the parent object.
	auto parentObject = g_pState->Domain->ObjectTable->CopyLiveObject(parentHandle);
	if (!parentObject || parentObject->Type != ObjectClass::Vehicle) return;

	// If this parent has another parent, the root is that grandparent.
	// (case: biped is sitting in a turret child of the root vehicle).
	uint32_t rootHandle = parentHandle;
	auto parentNodeIt = nodes.find(parentHandle);
	if (parentNodeIt != nodes.end() &&
		parentNodeIt->second.ParentHandle != 0xFFFFFFFF)
	{
		rootHandle = parentNodeIt->second.ParentHandle;
	}
	
	tree.VehicleHandle = rootHandle;
	
	auto rootIt = nodes.find(rootHandle);
	if (rootIt == nodes.end()) return;

	// We search for every vehicle objects class as child of the vehicle root.
	for (uint32_t childHandle : rootIt->second.ChildrenHandles)
	{
		auto childrenObject = 
			g_pState->Domain->ObjectTable->CopyLiveObject(childHandle);
	
		if (!childrenObject) continue;
		if (childrenObject->Type != ObjectClass::Vehicle) continue;
	
		tree.VehiclePartHandles.push_back(childHandle);
	}
}