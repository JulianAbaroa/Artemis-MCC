export module Blam.MapModel.Definitions.Type:Weap;

import :Object;

import std;

export namespace Blam::MapModel::Definitions::Type::Weap
{
    // Ammunition pool of a weapon. The datum keeps one magazine state per entry (weap 0x2C0, 0x2DA).
    struct Magazine
    {
        std::int16_t RoundsRecharged{};
        std::int16_t RoundsTotalInitial{};
        std::int16_t RoundsTotalMaximum{};
        std::int16_t RoundsLoadedMaximum{};
        std::int16_t RoundsReloaded{};
    };

    // Input of a weapon. The datum keeps one trigger state per entry (weap 0x2A8, 0x2B4).
    struct Trigger
    {
        // Spew, Latch, LatchAutofire, LatchTether, Charge, LatchZoom, SpewCharge, SwordCharge.
        std::uint16_t Behavior{};

        // Fire, Charge, FireOther.
        std::uint16_t PrimaryAction{};
        std::uint16_t SecondaryAction{};

        std::int16_t PrimaryBarrelIndex{ -1 };
        std::int16_t SecondaryBarrelIndex{ -1 };

        // Seconds to full charge, then seconds it can stay charged before the overcharged action.
        float ChargingTime{};
        float ChargedTime{};

        // None, Explode, Discharge.
        std::uint8_t OverchargedAction{};

        // Bit 0 can fire from partial charge.
        std::uint8_t ChargeFlags{};

        float ChargedDrainRate{};
    };

    // What a trigger fires.
    struct Barrel
    {
        float RoundsPerSecondMin{};
        float RoundsPerSecondMax{};

        // Index in Weap::Magazines, -1 for an energy weapon.
        std::int16_t MagazineIndex{ -1 };

        std::int16_t RoundsPerShot{};
        std::int16_t MinimumRoundsLoaded{};
        std::int16_t ProjectilesPerShot{};

        float FireRecoveryTime{};
        float HeatGeneratedPerRound{};

        std::uint8_t DamageReportingType{};
    };

    // Holds the weapon definition.
    struct Weap
    {
        Object::Object Base{};

        float AutoaimRange{};

        // Undefined, Shotgun, Needler, PlasmaPistol, PlasmaRifle, RocketLauncher, EnergyBlade, Splaser...
        std::uint16_t WeaponType{};

        float ReadyTime{};

        // Overheat with hysteresis: enters above OverheatedThreshold, leaves below HeatRecoveryThreshold.
        float HeatRecoveryThreshold{};
        float OverheatedThreshold{};
        float HeatLossPerSecond{};
        float OverheatedHeatLossPerSecond{};
        float HeatVentingTime{};

        // Seconds to lock on, 0 without target tracking.
        float TrackingAcquireTime{};
        bool HasTargetTracking{};

        std::vector<Magazine> Magazines{};
        std::vector<Trigger> Triggers{};
        std::vector<Barrel> Barrels{};
    };
}