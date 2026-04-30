#pragma once

#include <cstdint>
#include <string>

enum class EquipmentType : uint8_t
{
    Unknown,
    Sprint,
    Jetpack,
    HealthPack,
    AmmoPack,
    Hologram,
    Invincibility,
    RepulsorField,
    ProximityMine,
    MotionTrackerNoise,
    PowerFist,
    Spawner,
    Powerup,
    SuperJump,
};

struct EquipmentData
{
    std::string TagName;
    EquipmentType Type;

    // Use and duration.
    float WarmupTime;
    float Duration;          // negative = infinity
    float CooldownTime;
    int16_t Charges;           // -1 = unlimited
    bool IsToggle;          // false = Hold

    // Energy.
    float MinimumActivationEnergy;
    float ActiveEnergyRate;
    float RechargeRate;

    // AI Behavior.
    float DangerRadius;
    bool IsDangerousToAI;
    bool NeverDroppedByAI;
};