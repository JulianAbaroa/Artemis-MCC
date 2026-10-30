export module Perception.Details.Type:Projectile;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace Perception::Details::Type
{
    // Who fired a projectile and what it is stuck to, of one live object of Kind Projectile (D254).
    // note: The pose and the team are in Entities. Every id is a handle, k_NoHandle when there is none.
    struct ProjectileDetail
    {
        std::uint32_t Handle{};

        // Player that fired it, also from a vehicle seat and once stuck.
        std::uint32_t OwnerPlayer{ k_NoHandle };

        // Biped that fired it.
        std::uint32_t OwnerBiped{ k_NoHandle };

        // Root of the shooter's parent chain: the biped on foot, the root vehicle when seated (so it differs from OwnerBiped then).
        std::uint32_t ShooterRoot{ k_NoHandle };

        // Object the projectile is stuck to (a plasma grenade on a biped). k_NoHandle in flight and while InHand.
        std::uint32_t StuckTo{ k_NoHandle };

        // Node of StuckTo the projectile hangs from, -1 when it is not stuck.
        std::int8_t StuckNode{ -1 };

        // The grenade is still in the thrower's hand: its parent is the biped that threw it.
        // note: Measured on the armas_a golden (W sec. 2.3): a plasma grenade is parented to its own thrower's biped for about 12 ticks
        // before it is released, so that parent is not a victim.
        bool InHand{};
    };

    struct ProjectileDetails : std::unordered_map<std::uint32_t, ProjectileDetail>
    {
        using unordered_map::unordered_map;
    };
}