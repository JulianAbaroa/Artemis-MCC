export module Resolved.Definitions.Type:Jpt;

import std;

export namespace Resolved::Definitions::Type::Jpt
{
    // Holds the damage effect definition.
    struct Jpt
    {
        std::string TagName{};

        // None, Falling, Bullet, Grenade, HighExplosive, Sniper, Melee, Flame, MountedWeapon, Vehicle, Plasma, Needle, Shotgun, Assassinated.
        std::uint16_t Category{};

        // Bit 0 does not hurt owner, 1 can cause headshots, 7 only hurts shields, 9 skips shields, 15 does not hurt players.
        std::uint32_t DamageFlags{};

        // Damage points before the material and the distance scale.
        float DamageLowerBound{};
        float DamageUpperBoundMin{};
        float DamageUpperBoundMax{};
    };
}