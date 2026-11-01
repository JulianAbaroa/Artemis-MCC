export module Perception.Players.Type;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Team.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Player.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Team::Type::Team;
    using Blam::Runtime::Object::Type::Biped::ZoomLevel;
    using Blam::Runtime::Object::Type::Biped::GrenadeType;
    using Blam::Runtime::Player::Type::Alive::ConnectionState;

    using Common::Engine::Type::k_NoHandle;
}

export namespace Perception::Players::Type
{
    // What a biped touches with its feet (D260).
    enum class GroundKind : std::uint8_t
    {
        Air = 0,
        // The BSP.
        Static,
        // A living object, such as a forge block or a vehicle.
        Object,
        // The biped is parented to another object, such as a seat.
        Attached,
    };

    // Weapons, armor ability, grenades and objective of a live player (D93).
    // note: Read from the unit of the biped (A2), except the objective, which only the player table has.
    struct Loadout
    {
        // Weapons by slot, or 0xFFFFFFFF when empty. They do not swap when the player switches weapons.
        std::uint32_t PrimaryWeapon{ k_NoHandle };
        std::uint32_t SecondaryWeapon{ k_NoHandle };

        // Weapon in the hands.
        // note: 0xFFFFFFFF in a vehicle or without a usable weapon.
        std::uint32_t EquippedWeapon{ k_NoHandle };

        // Equipment of the armor ability, or 0xFFFFFFFF without one.
        std::uint32_t Ability{ k_NoHandle };

        // Energy left of the armor ability, from 0 (depleted) to 1 (full).
        float AbilityEnergy{};

        // Whether a sustained armor ability is running (camo, armor lock, jetpack, sprint).
        // note: Drop shield, hologram and evade are instant and never read active.
        bool IsAbilityActive{};

        GrenadeType SelectedGrenade{};
        std::uint8_t FragGrenades{};
        std::uint8_t PlasmaGrenades{};

        // Objective item carried, or 0xFFFFFFFF without one.
        std::uint32_t Objective{ k_NoHandle };
    };

    // State of the live biped of a player that its Entity does not have (D93).
    // note: The pose, the velocities, the health (Entity.Vitality) and the vehicle (Binding Seated and BoundTo) are in the Entity of BipedHandle.
    struct Body
    {
        // Aim direction of the unit, smoothed by the engine towards the desired aim.
        Vec3 Aim{};

        ZoomLevel Zoom{ ZoomLevel::None };

        // Ticks since the biped spawned.
        std::uint32_t TicksAlive{};

        // Vitality of the shield, from 0 to 1. Empty when the layout has no shield section.
        // note: The first section marked as shield. The engine ignores the other shield sections of a biped.
        std::optional<float> Shield{};

        Type::Loadout Loadout{};

        // Player of the latest hit since the shield last started recharging, or 0xFFFFFFFF.
        // note: The player itself after its own grenade or a fall.
        std::uint32_t LastDamagerPlayer{ k_NoHandle };

        // Ticks since the last damage, or empty if the biped was never damaged.
        std::optional<std::uint32_t> TicksSinceDamage{};

        // What the feet touch. A contact that cuts for 1 or 2 ticks between blocks keeps the last Ground (hysteresis, D260).
        GroundKind Ground{ GroundKind::Air };

        // Whether the biped can jump or walk now: true with contact, false when attached, and false in the air after 3 ticks or after a jump.
        bool IsGrounded{};

        // Ticks without contact in a row, 0 with contact or attached.
        std::uint32_t AirborneTicks{};

        // Object under the feet when Ground is Object, or 0xFFFFFFFF.
        // note: The engine drops it to 0xFFFFFFFF when the biped stands still on the object. The last valid one of this biped is kept.
        std::uint32_t GroundObject{ k_NoHandle };

        // Velocity of the biped relative to the object under its feet, or its own velocity when there is none.
        Vec3 OwnVelocity{};

        // Crouch pose, from 0 (standing) to 1 (crouched).
        float CrouchFraction{};

        // CrouchFraction >= 0.5.
        bool IsCrouching{};
    };

    // What the players service remembers of one biped between ticks to resolve its ground (D260).
    struct GroundMemoryEntry
    {
        std::uint32_t AirborneTicks{};

        // Ground of the last contact, or Attached while attached.
        GroundKind LastGround{ GroundKind::Air };

        // Last valid object of this biped while its Object contact did not cut.
        std::uint32_t LastObject{ k_NoHandle };

        // Whether the biped jumped off the ground: it stays until the next contact.
        bool IsTakeoff{};
    };

    // Ground memory by biped handle. Only the bipeds with a Body in the last tick are in it.
    struct GroundMemory : std::unordered_map<std::uint32_t, GroundMemoryEntry>
    {
        using unordered_map::unordered_map;
    };

    // One player of the player table with its body and loadout (D19, D30, D93).
    // note: IsAgent waits for the multi-view (B5 Views) and the seat for A11. The life of the biped is Entity.Vitality, there is no Body.Health (D257).
    struct Player
    {
        std::uint32_t Handle{};

        // Gamertag, or the service tag when it is empty. Debug only.
        std::string Name{};

        ConnectionState Connection{};

        // Local player (split-screen input user, 0 to 3), or empty when the player is not local (A9 M3).
        std::optional<std::uint8_t> LocalIndex{};

        // From the player table only (D54).
        ::Team Team{ ::Team::Neutral };

        // Current biped, alive or a corpse, or 0xFFFFFFFF without one.
        std::uint32_t BipedHandle{ k_NoHandle };

        // Ticks left until the player respawns, 0 while alive or without wait. Valid without a Body (D258).
        std::uint32_t RespawnTicks{};

        Vec3 CameraPosition{};
        Vec3 CameraForward{};

        // Only while the player has a live biped.
        std::optional<Type::Body> Body{};

        // return: Whether the player has a live biped.
        auto IsAlive() const -> bool
        {
            return Body.has_value();
        }
    };

    // Every player of the player table, sorted by handle, with the row of each handle.
    struct Players
    {
        std::vector<Player> Rows{};
        std::unordered_map<std::uint32_t, std::size_t> Index{};

        // return: The player of the handle, or null if it is not in the table.
        auto Find(std::uint32_t handle) const -> const Player*
        {
            const auto it{ Index.find(handle) };
            return it == Index.end() ? nullptr : &Rows[it->second];
        }
    };
}