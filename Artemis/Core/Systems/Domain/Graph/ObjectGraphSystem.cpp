#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Graph/ObjectGraphState.h"
#include "Core/States/Domain/Tables/ObjectTableState.h"
#include "Core/States/Domain/Tables/PlayerTableState.h"
#include "Core/States/Domain/Map/Phmo/MapPhmoState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/Graph/ObjectGraphSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

namespace
{
	const std::unordered_map<std::string, TagGroup> k_ClassToTagGroup =
	{
		{ "vehi", TagGroup::Vehicle      },
		{ "weap", TagGroup::Weapon       },
		{ "eqip", TagGroup::Equipment    },
		{ "scen", TagGroup::Scenery      },
		{ "ctrl", TagGroup::Control      },
		{ "bloc", TagGroup::Crate        },
		{ "proj", TagGroup::Projectile   },
	};

	TagGroup ResolveTagGroup(const std::string& cls)
	{
		auto it = k_ClassToTagGroup.find(cls);
		return it != k_ClassToTagGroup.end() ? it->second : TagGroup::Unknown;
	}
}

void ObjectGraphSystem::UpdateGraph()
{
	std::unordered_map<uint32_t, ObjectNode> nodes;
	this->BuildNodes(nodes);

	std::vector<PlayerObjectTree> trees;
	this->BuildPlayerTrees(nodes, trees);

	g_pState->Domain->ObjectGraph->SetNodes(std::move(nodes));
	g_pState->Domain->ObjectGraph->SetPlayerTrees(std::move(trees));
}


void ObjectGraphSystem::Cleanup()
{
	g_pState->Domain->ObjectGraph->Cleanup();
	g_pSystem->Debug->Log("[ObjectGraphSystem] INFO: Cleanup completed.");
}


void ObjectGraphSystem::BuildNodes(
	std::unordered_map<uint32_t, ObjectNode>& nodes)
{
	const auto& allObjects = g_pState->Domain->ObjectTable->GetAllObjects();

	for (const auto& [handle, object] : allObjects)
	{
		ObjectNode node{};
		node.Handle = handle;
		node.TagName = object.TagName;
		node.Class = object.Class;
		node.ParentHandle = object.ParentHandle;
		node.ChildHandle = object.ChildHandle;
		node.NextSiblingHandle = object.NextSiblingHandle;

		std::memcpy(node.Position, object.Position, sizeof(node.Position));
		std::memcpy(node.Forward, object.Forward, sizeof(node.Forward));
		std::memcpy(node.Up, object.Up, sizeof(node.Up));

		// Build TagProfile, classification base for AI.
		TagProfile profile;
		profile.PrimaryGroup = ResolveTagGroup(object.Class);

		// Sub-tags.
		//profile.HasPhmo = g_pState->Domain->MapPhmo->HasPhmo(object.TagName);

		node.Profile = profile;

		nodes.emplace(handle, node);
	}
}

void ObjectGraphSystem::BuildPlayerTrees(
	const std::unordered_map<uint32_t, ObjectNode>& nodes,
	std::vector<PlayerObjectTree>& trees)
{
	const auto& allPlayers = g_pState->Domain->PlayerTable->GetAllPlayers();

	for (const auto& [playerHandle, player] : allPlayers)
	{
		if (!player.Handle) continue;

		PlayerObjectTree tree{};
		tree.PlayerHandle = playerHandle;
		tree.Gamertag = player.Gamertag;

		tree.BipedHandle = player.AliveBipedHandle;
		tree.PrimaryWeaponHandle = player.PrimaryWeaponHandle;
		tree.SecondaryWeaponHandle = player.SecondaryWeaponHandle;
		tree.ObjectiveHandle = player.ObjectiveHandle;

		if (tree.BipedHandle != 0xFFFFFFFF)
		{
			auto it = nodes.find(tree.BipedHandle);
			if (it != nodes.end())
			{
				tree.VehicleHandle = it->second.ParentHandle;

				uint32_t currentSibling = it->second.NextSiblingHandle;
				while (currentSibling != 0xFFFFFFFF)
				{
					tree.VehicleSiblingHandles.push_back(currentSibling);

					auto sibIt = nodes.find(currentSibling);
					if (sibIt == nodes.end()) break;
					currentSibling = sibIt->second.NextSiblingHandle;
				}
			}
		}

		trees.push_back(std::move(tree));
	}
}

ObjectNode* ObjectGraphSystem::Resolve(
	std::unordered_map<uint32_t, ObjectNode>& nodes, uint32_t handle)
{
	if (handle == 0xFFFFFFFF) return nullptr;
	auto it = nodes.find(handle);
	return it != nodes.end() ? &it->second : nullptr;
}