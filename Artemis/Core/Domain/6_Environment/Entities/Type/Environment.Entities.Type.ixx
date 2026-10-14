export module Environment.Entities.Type;

import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Group;
    using Common::Semantic::Type::Lifecycle;
    using Common::Semantic::Type::Binding;
    using Common::Semantic::Type::Physics;
    using Common::Semantic::Type::Capability;
    using Common::Team::Type::Team;
    using Resolved::Definitions::Type::Object::ObjectKind;
}

export namespace Environment::Entities::Type
{
    // One live object with its meaning: classification, state, team, pose and capabilities (Ignore/Semantizacion/05, section 9).
    // note: FirstSeenTick waits for a global game tick (A8, D82) and OwnerPlayer for the owners of projectiles and bubbles (A3, Q48).
    struct Entity
    {
        std::uint32_t Handle{};

        ::ObjectKind Kind{ ::ObjectKind::Invalid };
        ::Function Function{ ::Function::Unknown };

        // Value of the subtype enum of the function (Subtype::<Function>). 0 is None.
        std::uint8_t Subtype{};
        ::Group Group{ ::Group::Unknown };

        // Dead when the first damage section has no vitality left (D85). Objects without vitality are always Active.
        ::Lifecycle Lifecycle{ ::Lifecycle::Active };

        // Link to the parent and the parent handle, or 0xFFFFFFFF when Free.
        ::Binding Binding{ ::Binding::Free };
        std::uint32_t BoundTo{ 0xFFFFFFFF };
        std::uint32_t ParentHandle{ 0xFFFFFFFF };

        // From the multiplayer block of the object only (D54). Neutral without it.
        ::Team Team{ ::Team::Neutral };

        // Forge physics of the placement (D79). None without placement.
        ::Physics Physics{ ::Physics::None };

        Capability Capabilities{ Capability::None };

        Vec3 BoundingCenter{};
        float BoundingRadius{};
        Vec3 WorldOrigin{};
        Vec3 Forward{};
        Vec3 Up{};
        Vec3 LinearVelocity{};
        Vec3 AngularVelocity{};

        // Vitality of the first damage section, from 0 to 1 (VitalityDetails, D85).
        // note: Only objects with HasVitality and damage sections in the datum.
        std::optional<float> Vitality{};
    };

    // Every live object as an entity, sorted by handle, with the row of each handle.
    struct Entities
    {
        std::vector<Entity> Rows{};
        std::unordered_map<std::uint32_t, std::size_t> Index{};

        // return: The entity of the handle, or null if it is not live.
        auto Find(std::uint32_t handle) const -> const Entity*
        {
            const auto it{ Index.find(handle) };
            return it == Index.end() ? nullptr : &Rows[it->second];
        }
    };
}