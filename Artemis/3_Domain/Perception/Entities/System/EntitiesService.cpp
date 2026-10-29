module Perception.Entities.System;

import Common.Engine.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Player.Type;
import Perception.ObjectGraph.Type;

namespace
{
    namespace Subtype = Common::Semantic::Type::Subtype;

    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Lifecycle;
    using Common::Semantic::Type::Binding;
    using Common::Semantic::Type::Physics;
    using Common::Semantic::Type::Capability;
    using Common::Semantic::Type::HasCapability;
    using Common::Team::Type::Team;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::Runtime::Object::Type::Placement::Placement;
    using Blam::Runtime::Player::Type::Alive::PlayerTable;
    using Perception::Details::Type::VitalityDetails;

    using Perception::Details::Type::k_LifeSection;
    using Common::Engine::Type::k_NoHandle;
    using Perception::ObjectGraph::Type::k_MaxParentDepth;

    // Removes a capability from a set.
    auto Without(Capability set, Capability capability) -> Capability
    {
        return static_cast<Capability>(static_cast<std::uint16_t>(set) & ~static_cast<std::uint16_t>(capability));
    }

    // Checks whether an object moves with physics on its own (05, section 6, D79, D80).
    // note: The Forge physics of its placement decides, and only Normal moves. Without placement, the kinds that always move.
    auto IsOwnDynamic(ObjectKind kind, const std::optional<Placement>& placement) -> bool
    {
        if (placement) return placement->Physics == Physics::Normal;

        switch (kind)
        {
        case ObjectKind::Biped:
        case ObjectKind::Vehicle:
        case ObjectKind::Weapon:
        case ObjectKind::Equipment:
        case ObjectKind::Projectile: return true;
        default: return false;
        }
    }

    // Checks whether the function is an item a biped picks up (05, section 6).
    auto IsItem(Function function) -> bool
    {
        switch (function)
        {
        case Function::Weapon:
        case Function::ObjectiveItem:
        case Function::ArmorAbility:
        case Function::Grenade:
        case Function::AmmoPack:
        case Function::Powerup: return true;
        default: return false;
        }
    }

    // Returns the vitality of the section that kills the object, from 0 to 1 (D85).
    // return: Empty when the object has no vitality detail.
    auto FindLife(const VitalityDetails& vitalities, std::uint32_t handle) -> std::optional<float>
    {
        const auto it{ vitalities.find(handle) };
        if (it == vitalities.end()) return std::nullopt;

        return it->second.Sections[k_LifeSection];
    }

    // Maps every biped of a player, alive or a corpse, to the team of the player (D94).
    auto CollectBipedTeams(const PlayerTable& playerTable) -> std::unordered_map<std::uint32_t, Team>
    {
        std::unordered_map<std::uint32_t, Team> teams{};
        teams.reserve(playerTable.size() * 2);

        for (const auto& [handle, player] : playerTable)
        {
            for (const std::uint32_t biped : { player.AliveBipedHandle, player.DeadBipedHandle, player.CurrentBipedHandle })
            {
                if (biped != k_NoHandle) teams[biped] = player.Team;
            }
        }

        return teams;
    }

    // Checks whether a weapon belongs to a vehicle: mounted guns and horns are never picked up (D74).
    auto IsVehicleWeapon(Function function, std::uint8_t subtype) -> bool
    {
        if (function != Function::Weapon) return false;

        return subtype == static_cast<std::uint8_t>(Subtype::Weapon::Mounted)
            || subtype == static_cast<std::uint8_t>(Subtype::Weapon::Horn);
    }
}

namespace Perception::Entities::System
{
    auto EntitiesService::Update() -> void
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        const ObjectTable& objectTable{ *objectTablePtr };

        // note: Before the first publish of the vitalities every object is Active.
        const auto vitalitiesPtr{ m_VitalityDetailsStore.Acquire() };
        const VitalityDetails empty{};
        const VitalityDetails& vitalities{ vitalitiesPtr ? *vitalitiesPtr : empty };

        // note: Before the first publish of the player table every biped is Neutral.
        const auto playerTablePtr{ m_PlayerStore.Acquire() };
        const auto bipedTeams{ playerTablePtr ? CollectBipedTeams(*playerTablePtr) : std::unordered_map<std::uint32_t, Team>{} };

        Entities entities{};
        entities.Rows.reserve(classifiedsPtr->size());

        for (const Classified& classified : *classifiedsPtr)
        {
            const auto objectIt{ objectTable.find(classified.Handle) };
            if (objectIt == objectTable.end()) continue;

            if (classified.Binding == Binding::Carried) m_CarriedHandlesStore.Add(classified.Handle);

            entities.Rows.push_back(this->MakeEntity(classified, objectIt->second, objectTable, vitalities, bipedTeams));
        }

        std::ranges::sort(entities.Rows, {}, &Entity::Handle);

        entities.Index.reserve(entities.Rows.size());
        for (std::size_t row{ 0 }; row < entities.Rows.size(); ++row)
        {
            entities.Index.emplace(entities.Rows[row].Handle, row);
        }

        m_EntitiesStore.Publish(std::move(entities));
    }

    auto EntitiesService::MakeEntity(const Classified& classified, const AliveObject& object, const ObjectTable& objectTable,
        const VitalityDetails& vitalities, const std::unordered_map<std::uint32_t, Team>& bipedTeams) -> Entity
    {
        Entity entity{};
        entity.Handle = classified.Handle;
        entity.Kind = classified.Kind;
        entity.Function = classified.Function;
        entity.Subtype = classified.Subtype;
        entity.Group = classified.Group;
        entity.Binding = classified.Binding;
        entity.BoundTo = classified.BoundTo;
        entity.ParentHandle = object.ParentHandle;
        entity.Team = object.Multiplayer ? object.Multiplayer->Team : Team::Neutral;

        // note: The multiplayer block of a biped reads Red for every player (D94). A biped without a player is Neutral.
        if (entity.Kind == ObjectKind::Biped)
        {
            const auto teamIt{ bipedTeams.find(entity.Handle) };
            entity.Team = teamIt == bipedTeams.end() ? Team::Neutral : teamIt->second;
        }
        entity.Physics = object.Placement ? object.Placement->Physics : Physics::None;
        entity.BoundingCenter = object.BoundingCenter;
        entity.BoundingRadius = object.BoundingRadius;
        entity.WorldOrigin = object.WorldOrigin;
        entity.Forward = object.Forward;
        entity.Up = object.Up;
        entity.LinearVelocity = object.LinearVelocity;
        entity.AngularVelocity = object.AngularVelocity;

        Capability capabilities{ classified.Capabilities };

        entity.Vitality = FindLife(vitalities, entity.Handle);

        // note: A dead object cannot die again, so it is no longer destructible.
        if (entity.Vitality && *entity.Vitality <= 0.0f) entity.Lifecycle = Lifecycle::Dead;
        if (HasCapability(capabilities, Capability::HasVitality) && entity.Lifecycle == Lifecycle::Active)
        {
            capabilities = capabilities | Capability::IsDestructible;
        }

        // note: A child follows the root of its parent chain: a carried weapon or a seated biped moves with its holder.
        const AliveObject& root{ this->RootOf(object, objectTable) };
        if (IsOwnDynamic(root.Profile.ObjectKind, root.Placement)) capabilities = capabilities | Capability::IsDynamic;

        // note: A destroyed vehicle, and the turrets attached to it, cannot be entered. Free and occupied seats are per seat (Details, View).
        if (HasCapability(capabilities, Capability::IsEnterable))
        {
            const bool isRootDead{ &root != &object && FindLife(vitalities, root.Handle).value_or(1.0f) <= 0.0f };
            if (entity.Lifecycle == Lifecycle::Dead || isRootDead) capabilities = Without(capabilities, Capability::IsEnterable);
        }

        // note: A Fixed or Phased item stays where it was dropped and cannot be picked up again (D80).
        const bool isStatic{ entity.Physics == Physics::Fixed || entity.Physics == Physics::Phased };
        const bool isDropped{ isStatic && m_CarriedHandlesStore.Contains(entity.Handle) };

        if (IsItem(entity.Function) && entity.Binding == Binding::Free && !IsVehicleWeapon(entity.Function, entity.Subtype) && !isDropped)
        {
            capabilities = capabilities | Capability::IsPickable;
        }

        entity.Capabilities = capabilities;

        return entity;
    }

    auto EntitiesService::RootOf(const AliveObject& object, const ObjectTable& objectTable) const -> const AliveObject&
    {
        const AliveObject* root{ &object };

        for (int depth{ 0 }; depth < k_MaxParentDepth && root->ParentHandle != k_NoHandle; ++depth)
        {
            const auto parentIt{ objectTable.find(root->ParentHandle) };
            if (parentIt == objectTable.end()) break;

            root = &parentIt->second;
        }

        return *root;
    }

    auto EntitiesService::Cleanup() -> void
    {
        m_EntitiesStore.Cleanup();
        m_CarriedHandlesStore.Cleanup();

        m_Log.Message("[EntitiesService] INFO: Cleanup completed.");
    }
}