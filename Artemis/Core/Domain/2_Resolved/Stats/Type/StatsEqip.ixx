export module Resolved.Stats.Type:Eqip;

import std;

export namespace Resolved::Stats::Type::Eqip
{
    enum class Type : std::uint8_t
    {
        Unknown,
        Sprint,
        Jetpack,
        Hologram,
        Invincibility,
        MotionTrackerNoise,
        ProximityMine,
        Spawner,
        Powerup,
        TreeOfLife,
        SpecialMove,
    };

    struct Eqip
    {
        std::string TagName;
        Type Type;

        // Timing
        float WarmupTime;
        float Duration; // -1 = infinite
        float EnergyRecoveryTime;
        std::int16_t Charges; // -1 = unlimited
        bool IsToggle; // false = Hold

        // Energy
        float MinimumActivationEnergy;
        float ActivationEnergyCost; // negative = costs energy
        float ActiveEnergyRate; // negative = drains while active
        float RechargeRate;
        float MovementSpeedDomain;

        // Flags
        bool CannotBeActiveAirborne;
        bool CannotActivateAirborne;
        bool CannotActivateInVehicle;
        bool DeactivatedByFiringWeapon;
        bool DeactivatedByMelee;
        bool DeactivatedByGrenade;
        bool ThirdPersonWhileActive;

        // AI
        float AwarenessTime;
        std::int8_t ObjectNoiseAdjustment;

        // Multiplayer pickup boundary (from MultiplayerObject[0] if present)
        bool HasMultiplayerObject;
        std::uint8_t MultiplayerObjectType; // 2=Grenade, 4=Powerup, 5=Equipment, 6=AmmoPack
        std::uint8_t BoundaryShape; // 0=None, 1=Sphere, 2=Cylinder, 3=Box
        float BoundaryWidthRadius;
        std::int16_t DefaultSpawnTime;
        std::int16_t DefaultAbandonmentTime;
    };
}