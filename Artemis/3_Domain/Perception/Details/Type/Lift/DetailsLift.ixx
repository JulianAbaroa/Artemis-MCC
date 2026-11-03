export module Perception.Details.Type:Lift;

import Common.Engine.Type;
import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Engine::Type::k_NoHandle;
}

export namespace Perception::Details::Type
{
    // Who a stage of a lift pushes (bits of LiftStage::Affects), from the flags of its phantom type (D98).
    enum class LiftAffects : std::uint8_t
    {
        Players = 1,
        // Bipeds that are not players.
        Bipeds = 2,
        Vehicles = 4,
        // Weapons, equipment and crates.
        Items = 8,
    };

    // One phantom type of the physics model of a lift: the push it applies inside its volume (D98).
    struct LiftStage
    {
        // Marker of the stage, world space. The origin of the lift when the type has no marker.
        Vec3 Position{};

        // +X of the marker, world space. The up of the lift when the type has no marker.
        Vec3 Direction{};

        // Along Direction, wu/s^2 and wu/s. The velocity is a cap: some lifts release the biped before it reaches it.
        float Acceleration{};
        float MaximumVelocity{};

        // Toward the axis of the marker, which centers the biped.
        float AxisAcceleration{};
        float AxisMaximumVelocity{};

        // LiftAffects bits.
        std::uint8_t Affects{};
    };

    // Stages and measured exit of one entity with Function Lift (D98, D261).
    // note: The kind (Vertical, Curved, Redirected) is the subtype of the entity and the pose is in Entities.
    struct LiftDetail
    {
        std::uint32_t Handle{};

        // One per phantom type that pushes players or bipeds, in the order of the tag. Empty without definitions or physics model.
        std::vector<LiftStage> Stages{};

        // A biped has left the push of this lift since the match began. The rest of the exit is only meaningful with it.
        bool HasExit{ false };

        // GameTick of the first tick with the biped out of the volume.
        std::uint32_t ExitTick{};

        // LinearVelocity of the biped at ExitTick, world space, wu/s. The last exit replaces the previous one.
        Vec3 ExitVelocity{};

        std::uint32_t ExitBiped{ k_NoHandle };
    };

    struct LiftDetails : std::unordered_map<std::uint32_t, LiftDetail>
    {
        using unordered_map::unordered_map;
    };

    // What the lift service remembers of a biped that is inside a phantom volume.
    struct LiftBipedEntry
    {
        // Lift the biped was assigned when it entered, k_NoHandle when none was near (doors and other phantoms).
        std::uint32_t Lift{ k_NoHandle };
    };

    // Bipeds inside a phantom volume in the previous tick, by handle. Only the ones that are inside are in it.
    struct LiftBipeds : std::unordered_map<std::uint32_t, LiftBipedEntry>
    {
        using unordered_map::unordered_map;
    };

    // Last exit of a lift.
    struct LiftExit
    {
        std::uint32_t Tick{};
        Vec3 Velocity{};
        std::uint32_t Biped{ k_NoHandle };
    };

    // Last exit of each lift that has one, by lift handle.
    struct LiftExits : std::unordered_map<std::uint32_t, LiftExit>
    {
        using unordered_map::unordered_map;
    };

    // The memory of the lift service between ticks (D261).
    struct LiftMemory
    {
        LiftBipeds Bipeds{};
        LiftExits Exits{};
    };
}