export module Blam.Runtime.Object.Type:Weapon;

import Common.Engine.Type;
import Common.Semantic.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;

    // Magazines the datum of a weapon holds.
    constexpr std::size_t k_MagazineCount{ 2 };
}

export namespace Blam::Runtime::Object::Type::Weapon
{
    // Reload or melee animation the weapon is playing.
    // note: Decoded from bits 5 and 6 of the action byte. The value 0x40 alone was never seen.
    enum class WeaponAction : std::uint8_t
    {
        None = 0x00,
        Reloading = 0x20,
        Meleeing = 0x60,
    };

    // Ammo of one magazine of the weapon.
    // note: Both read 0 on a magazine the weapon does not have, and on energy weapons.
    struct WeaponMagazine
    {
        // Rounds in the weapon, ready to fire.
        std::uint16_t Loaded{};

        // Rounds carried outside the weapon.
        std::uint16_t Reserve{};
    };

    struct Weapon
    {
        // Heat from 0 (cold) to 1.
        // note: Not the overheat state, see IsOverheated. The plasma rifle stays overheated with the heat below the tag threshold.
        float TotalHeat{};

        // The engine keeps the overheat state itself, in two phases: venting (bit 1 of the weapon flags) and cooling (bit 2).
        // note: Set while the heat is already below the tag threshold. The weapon can fire again when it clears.
        std::uint8_t IsOverheated{};

        // Energy left from 0 (empty) to 1 (full).
        // note: The engine stores the used fraction clamped to 0 and 1, so it is inverted when read.
        float TotalEnergy{};

        // Magazine 0 and magazine 1 (the second fire mode of the weapons that have one).
        // note: An array because the engine lays the two out the same way and the tag says how many the weapon has.
        std::array<WeaponMagazine, k_MagazineCount> Magazines{};

        // Fire key held. It is an input, so the weapon can fire without it ever being set.
        // note: A short tap fires but reads 0, like the other input flags.
        std::uint8_t IsTriggerHeld{};

        std::uint8_t IsZoomed{};

        // Reload or melee animation.
        // note: It returns to None when the animation ends, even if the key is still held.
        WeaponAction Action{};

        // Timer of the weapon being busy, non-zero while it cannot fire and 0 when it can.
        // note: Runs on a reload, when the weapon is drawn (about 38 ticks) and when it is stowed (about 7). Measured on armas_a.
        std::uint16_t BusyTimer{};

        // Reload state of magazine 0, raw.
        // note: 1 only during the reload and about 14 ticks longer, so a reload is this and BusyTimer both non-zero.
        std::uint16_t Magazine0ReloadState{};

        // Charge stage of trigger 0.
        Common::Semantic::Type::ChargeState ChargeState{};

        // The weapon tracks an object, with or without lock.
        std::uint8_t IsTracking{};

        // Object the weapon tracks, of any kind (a Falcon for the rocket launcher). k_NoHandle without target.
        std::uint32_t TrackedObject{ k_NoHandle };

        // Lock-on progress: 0 no target, 1 aimed, 2 locked.
        std::uint32_t TrackingState{};

        // Unit that holds the item, set on the tick it is taken and 0xFFFFFFFF when dropped. Only the flag is read for it today.
        // note: ParentHandle follows about 9 ticks later, so it cannot tell who took it.
        std::uint32_t HolderUnit{ k_NoHandle };

        // TODO: Fields of the vehicle weapons that are not read yet (the old Banshee offsets were removed).
        // - IsFiring: the float at 0x250 of the weapon ramps 0 -> 0.5 -> 1.0 -> 0.5 -> 0 while the
        //   weapon fires (the Banshee bomb gives a single 0.5 -> 0 pulse). Note that the horn of the
        //   Warthog is a weapon too and gives the same pulse. Analog Trigger (0x1CC) is the other candidate.
        // - IsCannonDepleted: probably Total Energy (0x1E4) or Total Heat (0x1E0) of the cannon.
        // - HasActiveBomb: not found. Maybe a live projectile owned by the bomb weapon.
    };
}