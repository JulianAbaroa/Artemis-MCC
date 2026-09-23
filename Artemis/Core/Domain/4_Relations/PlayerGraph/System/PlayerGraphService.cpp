module Relations.PlayerGraph.System;

import Relations.Classifier.Type;

namespace
{
	using Class = Tables::Object::Type::Class::Class;
	using Role = Relations::Classifier::Type::Role;
	using Classified = Relations::Classifier::Type::Classified;
	using Classifieds = std::vector<Classified>;
}

namespace Relations::PlayerGraph::System
{
	auto PlayerGraphService::UpdateGraph() -> void
	{
		auto objectTablePtr = m_ObjectStore.Acquire();
		auto playerTablePtr = m_PlayerStore.Acquire();
		if (!playerTablePtr || !objectTablePtr) return;

		PlayerTrees trees;
		this->BuildPlayerTrees(trees, *playerTablePtr, *objectTablePtr);
		m_PlayerGraphStore.Publish(std::move(trees));
	}

	auto PlayerGraphService::BuildPlayerTrees(PlayerTrees& trees,
		const PlayerTable& playerTable, const ObjectTable& objectTable) -> void
	{
		auto classifiedsPtr = m_ClassifierStore.Acquire();
		auto objectGraphPtr = m_ObjectGraphStore.Acquire();
		if (!objectGraphPtr || !classifiedsPtr) return;

		const ObjectNodes& objectNodes = *objectGraphPtr;
		const Classifieds& classifieds = *classifiedsPtr;

		for (const auto& [handle, player] : playerTable)
		{
			if (!player.Handle) continue;

			PlayerTree tree{};
			tree.Handle = handle;

			if (player.AliveBipedHandle != 0xFFFFFFFF)
			{
				if (objectNodes.contains(player.AliveBipedHandle))
				{
					tree.BipedHandle = player.AliveBipedHandle;
				}
			}

			if (player.PrimaryWeaponHandle != 0xFFFFFFFF)
			{
				if (objectNodes.contains(player.PrimaryWeaponHandle))
				{
					tree.PrimaryWeaponHandle = player.PrimaryWeaponHandle;
				}
			}
			if (player.SecondaryWeaponHandle != 0xFFFFFFFF)
			{
				if (objectNodes.contains(player.SecondaryWeaponHandle))
				{
					tree.SecondaryWeaponHandle = player.SecondaryWeaponHandle;
				}
			}

			if (player.ObjectiveHandle != 0xFFFFFFFF)
			{
				if (objectNodes.contains(player.ObjectiveHandle))
				{
					tree.ObjectiveHandle = player.ObjectiveHandle;
				}
			}

			for (const auto& classified : classifieds)
			{
				if (classified.Role != Role::ArmorAbilityEquipped)
				{
					continue;
				}

				auto it = objectNodes.find(classified.Handle);
				if (it == objectNodes.end()) continue;

				if (it->second.ParentHandle == tree.BipedHandle)
				{
					tree.AbilityHandle = classified.Handle;
					break;
				}
			}

			if (tree.BipedHandle != 0xFFFFFFFF)
			{
				this->BuildVehicle(tree, objectNodes, objectTable);
			}

			trees.push_back(std::move(tree));
		}
	}

	void PlayerGraphService::BuildVehicle(PlayerTree& playerTree,
		const ObjectNodes& objectNodes, const ObjectTable& objectTable)
	{
		auto bipedIt = objectNodes.find(playerTree.BipedHandle);
		if (bipedIt == objectNodes.end()) return;

		std::uint32_t parentHandle = bipedIt->second.ParentHandle;
		if (parentHandle == 0xFFFFFFFF) return;

		auto parentIt = objectTable.find(parentHandle);
		if (parentIt == objectTable.end() ||
			parentIt->second.Profile.Class != Class::Vehicle)
		{
			return;
		}

		std::uint32_t rootHandle = parentHandle;
		auto parentNodeIt = objectNodes.find(parentHandle);
		if (parentNodeIt != objectNodes.end() &&
			parentNodeIt->second.ParentHandle != 0xFFFFFFFF)
		{
			rootHandle = parentNodeIt->second.ParentHandle;
		}

		playerTree.ParentHandle = rootHandle;

		auto rootIt = objectNodes.find(rootHandle);
		if (rootIt == objectNodes.end()) return;

		for (std::uint32_t childHandle : rootIt->second.ChildrenHandles)
		{
			auto it = objectTable.find(childHandle);
			if (it == objectTable.end()) continue;
			auto& childrenObject = it->second;

			if (childrenObject.Profile.Class != Class::Vehicle)
			{
				continue;
			}

			playerTree.VehiclePartHandles.push_back(childHandle);
		}
	}

	void PlayerGraphService::Cleanup()
	{
		m_PlayerGraphStore.Cleanup();

		m_LogsService.Message("[PlayerGraphService] INFO: Cleanup completed.");
	}
}