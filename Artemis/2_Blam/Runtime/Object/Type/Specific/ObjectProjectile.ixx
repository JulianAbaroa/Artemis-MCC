export module Blam.Runtime.Object.Type:Projectile;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace Blam::Runtime::Object::Type::Projectile
{
    enum class ProjectileFlags : std::uint32_t
    {
        IsTracking = 0x00000100,
        IsCrit = 0x00400000,
        IsAltFire = 0x00800000,
    };

    struct Projectile
    {
        ProjectileFlags Flags{};
        // Player that fired the projectile, also from a vehicle seat and once stuck.
        std::uint32_t OwnerPlayer{ k_NoHandle };

        // Biped that fired it.
        std::uint32_t OwnerBiped{ k_NoHandle };

        // Root of the creator's parent chain: the biped on foot, the root vehicle when seated.
        std::uint32_t ShooterRoot{ k_NoHandle };

        // Node of the parent (the object it is stuck to) the projectile hangs from, -1 without parent.
        std::int8_t ParentNode{ -1 };
    };
}