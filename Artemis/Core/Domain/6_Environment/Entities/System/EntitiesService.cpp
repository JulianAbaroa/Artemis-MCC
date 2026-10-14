module Environment.Entities.System;

import Common.Semantic.Type;
import Common.Team.Type;
import Resolved.Definitions.Type;

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
    using Resolved::Definitions::Type::Object::ObjectKind;
    using Tables::Object::Type::Placement::Placement;

    // Value of an empty handle.
    constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };

    // Parents walked up to the root of an object. Guards against a broken chain.
    constexpr int k_MaxParentDepth{ 16 };

    // Damage section whose vitality kills the object at 0 (D85).
    // note: The sections the layout marks critical are not reliable: the headshot section of a Spartan stays at 1 after a body kill,
    // and the Ghost, the Falcon and the plasma battery have none.
    constexpr std::size_t k_LifeSection{ 0 };

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

    // Checks whether a weapon belongs to a vehicle: mounted guns and horns are never picked up (D74).
    auto IsVehicleWeapon(Function function, std::uint8_t subtype) -> bool
    {
        if (function != Function::Weapon) return false;

        return subtype == static_cast<std::uint8_t>(Subtype::Weapon::Mounted)
            || subtype == static_cast<std::uint8_t>(Subtype::Weapon::Horn);
    }
}

namespace Environment::Entities::System
{
    auto EntitiesService::Update() -> void
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        const ObjectTable& objectTable{ *objectTablePtr };

        Entities entities{};
        entities.Rows.reserve(classifiedsPtr->size());

        for (const Classified& classified : *classifiedsPtr)
        {
            const auto objectIt{ objectTable.find(classified.Handle) };
            if (objectIt == objectTable.end() || objectIt->second.Address == 0) continue;

            if (classified.Binding == Binding::Carried) m_CarriedHandlesStore.Add(classified.Handle);

            entities.Rows.push_back(this->MakeEntity(classified, objectIt->second, objectTable));
        }

        std::ranges::sort(entities.Rows, {}, &Entity::Handle);

        entities.Index.reserve(entities.Rows.size());
        for (std::size_t row{ 0 }; row < entities.Rows.size(); ++row)
        {
            entities.Index.emplace(entities.Rows[row].Handle, row);
        }

        m_EntitiesStore.Publish(std::move(entities));
    }

    auto EntitiesService::MakeEntity(const Classified& classified, const AliveObject& object, const ObjectTable& objectTable) -> Entity
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
        entity.Physics = object.Placement ? object.Placement->Physics : Physics::None;
        entity.BoundingCenter = object.BoundingCenter;
        entity.BoundingRadius = object.BoundingRadius;
        entity.WorldOrigin = object.WorldOrigin;
        entity.Forward = object.Forward;
        entity.Up = object.Up;
        entity.LinearVelocity = object.LinearVelocity;
        entity.AngularVelocity = object.AngularVelocity;

        Capability capabilities{ classified.Capabilities };

        if (HasCapability(capabilities, Capability::HasVitality))
        {
            capabilities = capabilities | Capability::IsDestructible;
            entity.Vitality = this->FindVitality(object);
        }

        if (entity.Vitality && *entity.Vitality <= 0.0f) entity.Lifecycle = Lifecycle::Dead;

        // note: A child follows the root of its parent chain: a carried weapon or a seated biped moves with its holder.
        const AliveObject& root{ this->RootOf(object, objectTable) };
        if (IsOwnDynamic(root.Profile.ObjectKind, root.Placement)) capabilities = capabilities | Capability::IsDynamic;

        // note: A destroyed vehicle, and the turrets attached to it, cannot be entered. Free and occupied seats are per seat (Details, View).
        if (HasCapability(capabilities, Capability::IsEnterable))
        {
            const bool isRootDead{ &root != &object && this->FindVitality(root).value_or(1.0f) <= 0.0f };
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

    auto EntitiesService::FindVitality(const AliveObject& object) const -> std::optional<float>
    {
        const Vitality* layout{ m_VitalityStore.Get(object.TagName) };
        if (layout == nullptr || layout->Sections.empty()) return std::nullopt;

        const auto* damage{ m_DamageSectionsStore.Get(object.Handle) };
        if (damage == nullptr || damage->Sections.size() <= k_LifeSection) return std::nullopt;

        return damage->Sections[k_LifeSection].Vitality;
    }

    auto EntitiesService::Cleanup() -> void
    {
        m_EntitiesStore.Cleanup();
        m_CarriedHandlesStore.Cleanup();

        m_LogsService.Message("[EntitiesService] INFO: Cleanup completed.");
    }
}