#pragma once

#include <cstdint>
#include <string>

enum class EquipmentType : uint8_t
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

struct EquipmentData
{
    std::string TagName;
    EquipmentType Type;

    // Timing
    float WarmupTime;
    float Duration; // -1 = infinite
    float EnergyRecoveryTime;
    int16_t Charges; // -1 = unlimited
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
    int8_t ObjectNoiseAdjustment;

    // Multiplayer pickup boundary (from MultiplayerObject[0] if present)
    bool HasMultiplayerObject;
    uint8_t MultiplayerObjectType; // 2=Grenade, 4=Powerup, 5=Equipment, 6=AmmoPack
    uint8_t BoundaryShape; // 0=None, 1=Sphere, 2=Cylinder, 3=Box
    float BoundaryWidthRadius;
    int16_t DefaultSpawnTime;
    int16_t DefaultAbandonmentTime;
};