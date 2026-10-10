export module Tables.Object.Type:Weapon;

import Common.Team.Type;
import std;

namespace
{
    using Common::Team::Type::Team;
}

export namespace Tables::Object::Type::Weapon
{
    // Reload or melee animation the weapon is playing.
    // note: Decoded from bits 5 and 6 of the action byte. The value 0x40 alone was never seen.
    enum class WeaponAction : std::uint8_t
    {
        None = 0x00,
        Reloading = 0x20,
        Meleeing = 0x60,
    };

    // Stage of the charged fire mode.
    // note: Seen on the plasma pistol and the plasma launcher. Released was only seen on the plasma launcher. The stages still need to be measured with logs.
    enum class ChargeState : std::uint8_t
    {
        None = 0,
        Charging = 1,
        Charged = 2,
        Released = 3,
    };

    struct Weapon
    {
        // Heat from 0 (cold) to 1.
        // note: The plasma pistol overheats between 0.8 and 0.93 and is usable again near 0.2. Other weapons were not measured.
        float TotalHeat{};

        // Energy left from 0 (empty) to 1 (full).
        // note: The engine stores the used fraction clamped to 0 and 1, so it is inverted when read.
        float TotalEnergy{};

        // Ammo carried and ammo loaded in the magazine.
        // note: Both read 0 on energy weapons.
        std::uint16_t TotalAmmo{};
        std::uint16_t CurrentAmmo{};

        // Fire key held. It is an input, so the weapon can fire without it ever being set.
        // note: A short tap fires but reads 0, like the other input flags.
        std::uint8_t IsTriggerHeld{};

        std::uint8_t IsZoomed{};

        // Reload or melee animation.
        // note: It returns to None when the animation ends, even if the key is still held.
        WeaponAction Action{};

        // Reload in progress on magazine weapons.
        // note: It stays 1 after the weapon can fire again, until the reload fully ends.
        std::uint8_t IsReloading{};

        ChargeState ChargeState{};

        // Lock-on state of tracking weapons.
        // note: Not observed yet.
        std::uint8_t IsTracking{};
        std::uint32_t TrackedBipedHandle{};

        // Team of the flag or the bomb.
        // note: Only read for those two weapons.
        std::optional<Team> Team{};
    };
}