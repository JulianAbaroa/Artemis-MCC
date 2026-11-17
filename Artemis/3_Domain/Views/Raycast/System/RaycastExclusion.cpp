module Views.Raycast.System;

import Common.Engine.Type;
import Blam.MapModel.Definitions.Type;
import Perception.Entities.Type;
import Perception.ObjectGraph.Type;
import Perception.Players.Type;

namespace
{
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Perception::Entities::Type::Entities;
    using Perception::Entities::Type::Entity;
    using Perception::ObjectGraph::Type::ObjectNode;
    using Perception::ObjectGraph::Type::ObjectNodes;
    using Perception::Players::Type::Loadout;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Views::Raycast::Type::PlayerExclusions;
    using Views::Raycast::Type::SelfExclusion;

    using Common::Engine::Type::k_NoHandle;
    using Perception::ObjectGraph::Type::k_MaxParentDepth;
}

namespace Views::Raycast::System
{
    auto FindVehicleRoot(const Entities& entities, std::uint32_t bipedHandle) -> std::uint32_t
    {
        const Entity* current{ entities.Find(bipedHandle) };
        if (!current || current->ParentHandle == k_NoHandle) return k_NoHandle;

        for (int depth{ 0 }; depth < k_MaxParentDepth && current->ParentHandle != k_NoHandle; ++depth)
        {
            const Entity* parent{ entities.Find(current->ParentHandle) };
            if (!parent) break;

            current = parent;
        }

        return current->Kind == ObjectKind::Vehicle ? current->Handle : k_NoHandle;
    }

    auto BuildGraphExclusions(const Players& players, const Entities& entities, const ObjectNodes& nodes) -> PlayerExclusions
    {
        PlayerExclusions exclusions(players.Rows.size());

        for (std::size_t index{ 0 }; index < players.Rows.size(); ++index)
        {
            const Player& player{ players.Rows[index] };
            if (player.BipedHandle == k_NoHandle) continue;

            SelfExclusion& exclusion{ exclusions[index] };
            exclusion.insert(player.BipedHandle);

            if (player.IsAlive())
            {
                const Loadout& loadout{ player.Body->Loadout };
                for (const std::uint32_t handle : { loadout.PrimaryWeapon, loadout.SecondaryWeapon, loadout.Ability, loadout.Objective })
                {
                    if (handle != k_NoHandle) exclusion.insert(handle);
                }
            }

            const std::uint32_t vehicleHandle{ FindVehicleRoot(entities, player.BipedHandle) };
            if (vehicleHandle != k_NoHandle)
            {
                exclusion.insert(vehicleHandle);

                const ObjectNode* vehicleNode{ nodes.Find(vehicleHandle) };
                if (vehicleNode)
                {
                    for (const std::uint32_t child : nodes.ChildrenOf(*vehicleNode))
                    {
                        const Entity* entity{ entities.Find(child) };
                        if (entity && entity->Kind == ObjectKind::Vehicle) exclusion.insert(child);
                    }
                }
            }

            const ObjectNode* bipedNode{ nodes.Find(player.BipedHandle) };
            if (bipedNode)
            {
                for (const std::uint32_t child : nodes.ChildrenOf(*bipedNode)) exclusion.insert(child);
            }
        }

        return exclusions;
    }
}