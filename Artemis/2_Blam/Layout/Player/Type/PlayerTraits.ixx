export module Blam.Layout.Player.Type:Traits;

import std;

// Block of player traits (0x2C bytes) of the player table entry, the one the engine composes each tick from the base traits, the
// powerups, the carried objective and the respawn traits. The game variant has blocks of the same format (D262).
// note: Every field of the player block is resolved: the raw value 0 (Unchanged) should not appear in it.
export namespace Blam::Layout::Player::Type::Traits
{
    // Bytes of the block.
    constexpr std::size_t k_Size{ 0x2C };

    // Offsets of the fields inside the block.
    constexpr std::size_t k_DamageResistance{ 0x00 };
    constexpr std::size_t k_HealthMultiplier{ 0x01 };
    constexpr std::size_t k_HealthRegenRate{ 0x02 };
    constexpr std::size_t k_ShieldMultiplier{ 0x03 };
    constexpr std::size_t k_ShieldRegenRate{ 0x04 };
    constexpr std::size_t k_OvershieldRegenRate{ 0x05 };
    constexpr std::size_t k_HeadshotImmune{ 0x06 };
    constexpr std::size_t k_ShieldVampirism{ 0x07 };
    constexpr std::size_t k_AssassinationImmune{ 0x08 };
    constexpr std::size_t k_CannotDieFromDamage{ 0x09 };
    constexpr std::size_t k_DamageMultiplier{ 0x0C };
    constexpr std::size_t k_MeleeMultiplier{ 0x0D };
    constexpr std::size_t k_GrenadeRegen{ 0x0E };
    constexpr std::size_t k_WeaponPickup{ 0x0F };
    constexpr std::size_t k_GrenadeCount{ 0x10 };
    constexpr std::size_t k_InfiniteAmmo{ 0x11 };
    constexpr std::size_t k_AbilityUsage{ 0x12 };
    constexpr std::size_t k_AbilitiesDropOnDeath{ 0x13 };
    constexpr std::size_t k_InfiniteAbilities{ 0x14 };
    constexpr std::size_t k_PrimaryWeapon{ 0x15 };
    constexpr std::size_t k_SecondaryWeapon{ 0x16 };
    constexpr std::size_t k_Ability{ 0x17 };
    constexpr std::size_t k_MovementSpeed{ 0x18 };
    constexpr std::size_t k_Gravity{ 0x19 };
    constexpr std::size_t k_VehicleUse{ 0x1A };
    constexpr std::size_t k_JumpHeight{ 0x1C };
    constexpr std::size_t k_ActiveCamo{ 0x20 };
    constexpr std::size_t k_VisibleWaypoint{ 0x21 };
    constexpr std::size_t k_VisibleName{ 0x22 };
    constexpr std::size_t k_RadarMode{ 0x28 };
    constexpr std::size_t k_RadarRange{ 0x29 };

    // Meaning of a raw bool trait: 1 no, 2 yes.
    constexpr std::uint8_t k_BoolYes{ 2 };

    // Grenade count that means the map default, and the value the decoded counts take then.
    constexpr std::uint8_t k_GrenadeCountMapDefault{ 1 };
    constexpr std::uint8_t k_MapDefaultGrenades{ 255 };

    enum class InfiniteAmmoMode : std::uint8_t
    {
        Unknown = 0,
        Off,
        On,
        Bottomless,
    };

    enum class AbilityUsageMode : std::uint8_t
    {
        Unknown = 0,
        Disabled,
        NotWithObjectives,
        Enabled,
    };

    enum class VehicleUseMode : std::uint8_t
    {
        Unknown = 0,
        Blocked,
        PassengerOnly,
        DriverOnly,
        GunnerOnly,
        NoPassenger,
        NoDriver,
        NoGunner,
        Full,
    };

    // Who sees the waypoint or the name of the player.
    enum class IdentityVisibility : std::uint8_t
    {
        Unknown = 0,
        Hidden,
        Allies,
        Everyone,
    };

    enum class RadarDisplayMode : std::uint8_t
    {
        Unknown = 0,
        Off,
        Allies,
        Normal,
        Enhanced,
    };

    // Traits of a player, decoded from the block. Multipliers are 1.0 for 100%.
    // note: A raw index outside its table decodes to NaN (floats), false (bools) or Unknown (enums).
    struct PlayerTraits
    {
        // Infinity for the invulnerable setting.
        float DamageResistance{};
        float HealthMultiplier{};
        float HealthRegenRate{};
        float ShieldMultiplier{};
        float ShieldRegenRate{};
        float OvershieldRegenRate{};
        float ShieldVampirism{};
        // Weapon damage. Infinity for the instant kill setting.
        float DamageMultiplier{};
        // Infinity for the instant kill setting.
        float MeleeMultiplier{};
        float MovementSpeed{};
        float Gravity{};
        // Jump height as the multiplier of the stored percentage.
        float JumpHeight{};
        // Motion tracker range in meters, not a multiplier.
        float RadarRange{};

        bool IsHeadshotImmune{};
        bool IsAssassinationImmune{};
        bool CannotDieFromDamage{};
        bool IsGrenadeRegenOn{};
        bool IsWeaponPickupOn{};
        bool DoAbilitiesDropOnDeath{};
        bool AreAbilitiesInfinite{};

        InfiniteAmmoMode InfiniteAmmo{};
        AbilityUsageMode AbilityUsage{};
        VehicleUseMode VehicleUse{};
        IdentityVisibility VisibleWaypoint{};
        IdentityVisibility VisibleName{};
        RadarDisplayMode RadarMode{};

        // Grenades the player starts with (255 each for the map default).
        std::uint8_t FragGrenades{};
        std::uint8_t PlasmaGrenades{};
        // Raw setting: 1 off to 5 best.
        std::uint8_t ActiveCamo{};

        // Raw indices of the weapon and ability tables of the game variant editor: -1 none, 0 the first one.
        std::int8_t PrimaryWeapon{};
        std::int8_t SecondaryWeapon{};
        std::int8_t Ability{};
    };

    constexpr float k_NaN{ std::numeric_limits<float>::quiet_NaN() };
    constexpr float k_Infinity{ std::numeric_limits<float>::infinity() };

    // Tables raw value (from 1) to value of each numeric trait.
    constexpr std::array<float, 12> k_DamageResistanceTable{ 0.1f, 0.5f, 0.9f, 1.0f, 1.1f, 1.5f, 2.0f, 3.0f, 5.0f, 10.0f, 20.0f, k_Infinity };
    constexpr std::array<float, 6> k_HealthShieldMultiplierTable{ 0.0f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f };
    constexpr std::array<float, 9> k_HealthRegenTable{ -0.25f, -0.1f, -0.05f, 0.0f, 0.5f, 0.9f, 1.0f, 1.1f, 2.0f };
    constexpr std::array<float, 14> k_ShieldRegenTable{ -0.25f, -0.1f, -0.05f, 0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f, 1.1f, 1.25f, 1.5f, 2.0f };
    constexpr std::array<float, 5> k_VampirismTable{ 0.0f, 0.1f, 0.25f, 0.5f, 1.0f };
    constexpr std::array<float, 12> k_DamageMultiplierTable{ 0.0f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f, 1.1f, 1.25f, 1.5f, 2.0f, 3.0f, k_Infinity };
    constexpr std::array<float, 17> k_MovementSpeedTable{ 0.0f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, 2.0f, 3.0f };
    constexpr std::array<float, 5> k_GravityTable{ 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
    // Meters.
    constexpr std::array<float, 7> k_RadarRangeTable{ 10.0f, 15.0f, 25.0f, 50.0f, 75.0f, 100.0f, 150.0f };

    // Percentage divisor of the jump height.
    constexpr float k_JumpHeightPercent{ 100.0f };

    // return: The value of the raw index in a table, or NaN when the index is outside it.
    template <std::size_t N>
    constexpr auto Lookup(const std::array<float, N>& table, std::uint8_t raw) -> float
    {
        return raw >= 1 && raw <= N ? table[raw - 1] : k_NaN;
    }

    // return: The raw value as an enum, or Unknown (0) when it is above the last value.
    template <typename Enum>
    constexpr auto ToEnum(std::uint8_t raw, Enum last) -> Enum
    {
        return raw >= 1 && raw <= static_cast<std::uint8_t>(last) ? static_cast<Enum>(raw) : Enum::Unknown;
    }

    // return: Whether the raw bool trait is yes. Anything else is false.
    constexpr auto ToBool(std::uint8_t raw) -> bool
    {
        return raw == k_BoolYes;
    }

    // Decodes the raw grenade setting into the counts of each kind.
    // param raw: 1 map default, 2 none, 3..6 frag 1..4, 7..10 plasma 1..4, 11..14 both 1..4. Other values leave 0 and 0.
    constexpr auto DecodeGrenades(std::uint8_t raw, std::uint8_t& frag, std::uint8_t& plasma) -> void
    {
        frag = 0;
        plasma = 0;
        if (raw == k_GrenadeCountMapDefault)
        {
            frag = k_MapDefaultGrenades;
            plasma = k_MapDefaultGrenades;
        }
        else if (raw >= 3 && raw <= 6) frag = static_cast<std::uint8_t>(raw - 2);
        else if (raw >= 7 && raw <= 10) plasma = static_cast<std::uint8_t>(raw - 6);
        else if (raw >= 11 && raw <= 14)
        {
            frag = static_cast<std::uint8_t>(raw - 10);
            plasma = static_cast<std::uint8_t>(raw - 10);
        }
    }

    // return: The traits of a block.
    constexpr auto Decode(const std::array<std::uint8_t, k_Size>& block) -> PlayerTraits
    {
        PlayerTraits traits{};

        traits.DamageResistance = Lookup(k_DamageResistanceTable, block[k_DamageResistance]);
        traits.HealthMultiplier = Lookup(k_HealthShieldMultiplierTable, block[k_HealthMultiplier]);
        traits.HealthRegenRate = Lookup(k_HealthRegenTable, block[k_HealthRegenRate]);
        traits.ShieldMultiplier = Lookup(k_HealthShieldMultiplierTable, block[k_ShieldMultiplier]);
        traits.ShieldRegenRate = Lookup(k_ShieldRegenTable, block[k_ShieldRegenRate]);
        traits.OvershieldRegenRate = Lookup(k_ShieldRegenTable, block[k_OvershieldRegenRate]);
        traits.ShieldVampirism = Lookup(k_VampirismTable, block[k_ShieldVampirism]);
        traits.DamageMultiplier = Lookup(k_DamageMultiplierTable, block[k_DamageMultiplier]);
        traits.MeleeMultiplier = Lookup(k_DamageMultiplierTable, block[k_MeleeMultiplier]);
        traits.MovementSpeed = Lookup(k_MovementSpeedTable, block[k_MovementSpeed]);
        traits.Gravity = Lookup(k_GravityTable, block[k_Gravity]);
        traits.RadarRange = Lookup(k_RadarRangeTable, block[k_RadarRange]);

        const std::uint32_t jumpPercent{ static_cast<std::uint32_t>(block[k_JumpHeight]) | (static_cast<std::uint32_t>(block[k_JumpHeight + 1]) << 8) |
            (static_cast<std::uint32_t>(block[k_JumpHeight + 2]) << 16) | (static_cast<std::uint32_t>(block[k_JumpHeight + 3]) << 24) };
        traits.JumpHeight = static_cast<float>(static_cast<std::int32_t>(jumpPercent)) / k_JumpHeightPercent;

        traits.IsHeadshotImmune = ToBool(block[k_HeadshotImmune]);
        traits.IsAssassinationImmune = ToBool(block[k_AssassinationImmune]);
        traits.CannotDieFromDamage = ToBool(block[k_CannotDieFromDamage]);
        traits.IsGrenadeRegenOn = ToBool(block[k_GrenadeRegen]);
        traits.IsWeaponPickupOn = ToBool(block[k_WeaponPickup]);
        traits.DoAbilitiesDropOnDeath = ToBool(block[k_AbilitiesDropOnDeath]);
        traits.AreAbilitiesInfinite = ToBool(block[k_InfiniteAbilities]);

        traits.InfiniteAmmo = ToEnum(block[k_InfiniteAmmo], InfiniteAmmoMode::Bottomless);
        traits.AbilityUsage = ToEnum(block[k_AbilityUsage], AbilityUsageMode::Enabled);
        traits.VehicleUse = ToEnum(block[k_VehicleUse], VehicleUseMode::Full);
        traits.VisibleWaypoint = ToEnum(block[k_VisibleWaypoint], IdentityVisibility::Everyone);
        traits.VisibleName = ToEnum(block[k_VisibleName], IdentityVisibility::Everyone);
        traits.RadarMode = ToEnum(block[k_RadarMode], RadarDisplayMode::Enhanced);

        DecodeGrenades(block[k_GrenadeCount], traits.FragGrenades, traits.PlasmaGrenades);
        traits.ActiveCamo = block[k_ActiveCamo];

        traits.PrimaryWeapon = static_cast<std::int8_t>(block[k_PrimaryWeapon]);
        traits.SecondaryWeapon = static_cast<std::int8_t>(block[k_SecondaryWeapon]);
        traits.Ability = static_cast<std::int8_t>(block[k_Ability]);

        return traits;
    }
}