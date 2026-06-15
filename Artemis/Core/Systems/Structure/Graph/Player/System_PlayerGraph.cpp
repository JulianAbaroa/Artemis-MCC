#include "pch.h"

#include "System_PlayerGraph.h"

#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"
#include "Core/States/Sources/Tables/Player/State_PlayerTable.h"
#include "Core/States/Structure/Classifier/State_Classifier.h"
#include "Core/States/Structure/Graph/Object/State_ObjectGraph.h"
#include "Core/States/Structure/Graph/Player/State_PlayerGraph.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

void System_PlayerGraph::UpdateGraph()
{
	auto playerTablePtr = m_Deps.State_PlayerTable.Acquire();
	auto objectTablePtr = m_Deps.State_ObjectTable.Acquire();
	if (!playerTablePtr || !objectTablePtr) return;

	PlayerGraph playerGraph;
	this->BuildPlayerTrees(playerGraph, *playerTablePtr, *objectTablePtr);
	m_Deps.State_PlayerGraph.Publish(std::move(playerGraph));
}

void System_PlayerGraph::BuildPlayerTrees(PlayerGraph& trees,
	const PlayerTable& playerTable, const ObjectTable& objectTable)
{
	auto objectGraphPtr = m_Deps.State_ObjectGraph.Acquire();
	auto classifiedsPtr = m_Deps.State_Classifier.Acquire();
	if (!objectGraphPtr || !classifiedsPtr) return;

	const ObjectGraph& objectGraph = *objectGraphPtr;
	const Classifieds& classifieds = *classifiedsPtr;

	for (const auto& [handle, player] : playerTable)
	{
		if (!player.Handle) continue;

		PlayerTree tree{};
		tree.Handle = handle;

		if (player.AliveBipedHandle != 0xFFFFFFFF)
		{
			if (objectGraph.contains(player.AliveBipedHandle))
			{
				tree.BipedHandle = player.AliveBipedHandle;
			}
		}

		if (player.PrimaryWeaponHandle != 0xFFFFFFFF)
		{
			if (objectGraph.contains(player.PrimaryWeaponHandle))
			{
				tree.PrimaryWeaponHandle = player.PrimaryWeaponHandle;
			}
		}
		if (player.SecondaryWeaponHandle != 0xFFFFFFFF)
		{
			if (objectGraph.contains(player.SecondaryWeaponHandle))
			{
				tree.SecondaryWeaponHandle = player.SecondaryWeaponHandle;
			}
		}

		if (player.ObjectiveHandle != 0xFFFFFFFF)
		{
			if (objectGraph.contains(player.ObjectiveHandle))
			{
				tree.ObjectiveHandle = player.ObjectiveHandle;
			}
		}

		for (const auto& classified : classifieds)
		{
			if (classified.Role !=
				ObjectRole::ArmorAbilityEquipped)
			{
				continue;
			}

			auto it = objectGraph.find(classified.Handle);
			if (it == objectGraph.end()) continue;

			if (it->second.ParentHandle == tree.BipedHandle)
			{
				tree.AbilityHandle = classified.Handle;
				break;
			}
		}

		if (tree.BipedHandle != 0xFFFFFFFF)
		{
			this->BuildVehicle(tree, objectGraph, objectTable);
		}

		trees.push_back(std::move(tree));
	}
}

void System_PlayerGraph::BuildVehicle(PlayerTree& playerTree,
	const ObjectGraph& objectGraph, const ObjectTable& objectTable)
{
	auto bipedIt = objectGraph.find(playerTree.BipedHandle);
	if (bipedIt == objectGraph.end()) return;

	uint32_t parentHandle = bipedIt->second.ParentHandle;
	if (parentHandle == 0xFFFFFFFF) return;

	auto parentIt = objectTable.find(parentHandle);
	if (parentIt == objectTable.end() ||
		parentIt->second.Profile.Class != ObjectClass::Vehicle)
	{
		return;
	}

	uint32_t rootHandle = parentHandle;
	auto parentNodeIt = objectGraph.find(parentHandle);
	if (parentNodeIt != objectGraph.end() &&
		parentNodeIt->second.ParentHandle != 0xFFFFFFFF)
	{
		rootHandle = parentNodeIt->second.ParentHandle;
	}
	
	playerTree.ParentHandle = rootHandle;
	
	auto rootIt = objectGraph.find(rootHandle);
	if (rootIt == objectGraph.end()) return;

	for (uint32_t childHandle : rootIt->second.ChildrenHandles)
	{
		auto it = objectTable.find(childHandle);
		if (it == objectTable.end()) continue;
		auto& childrenObject = it->second;
	
		if (childrenObject.Profile.Class !=
			ObjectClass::Vehicle)
		{
			continue;
		}
	
		playerTree.VehiclePartHandles.push_back(childHandle);
	}
}

void System_PlayerGraph::Cleanup()
{
	m_Deps.State_PlayerGraph.Cleanup();

	m_Deps.System_Logs.Log("[PlayerGraph] INFO:"
		" Cleanup completed.");
}
