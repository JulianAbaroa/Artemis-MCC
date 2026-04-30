#pragma once

#include <string>

struct WeaponBarrelData
{
    // Cadence.
    float RoundsPerSecondMin;
    float RoundsPerSecondMax;

    // Precision.
    float MinimumError;
    float ErrorAngleMin;
    float ErrorAngleMax;

    // Projectiles.
    int16_t ProjectilesPerShot;

    // Noise for AI.
    uint16_t FiringNoise; // 0=Silent, 1=Quiet, 2=Medium, 3=Shout, 4=Loud
};

struct WeaponMagazineData
{
    int16_t RoundsTotalMaximum;
    int16_t RoundsLoadedMaximum;
};

struct WeaponData
{
    std::string TagName;

    // Classification.
    uint16_t WeaponType;   // raw enum: Undefined, Shotgun, Needler, etc.
    uint32_t WeaponClass;  // stringid raw
    uint32_t WeaponName;   // stringid raw
    int16_t  MagnificationLevels;
    bool HasTargetTracking;
    bool IsSupportWeapon;

    // Movement.
    float ForwardMovementPenalty;
    float SidewaysMovementPenalty;

    // Autoaim.
    float AutoaimAngle;
    float AutoaimRange;

    // Heat / overheating.
    float OverheatedThreshold;
    float HeatLossPerSecond;
    bool  CanOverheat; // OverheatedThreshold > 0

    // AI
    float AiScariness;

    // Primary Barrel (Barrels[0])
    bool HasBarrel;
    WeaponBarrelData PrimaryBarrel;

    // Primary Magazine (Magazines[0])
    bool HasMagazine;
    WeaponMagazineData PrimaryMagazine;
};