#pragma once

#include <string>

struct WeaponBarrelData
{
    // Rate of fire
    float RoundsPerSecondMin;
    float RoundsPerSecondMax;
    float AccelerationTime;
    float DecelerationTime;
    float FireRecoveryTime;

    // Accuracy
    float MinimumError;
    float ErrorAngleMin;
    float ErrorAngleMax;

    // Ammo
    int16_t RoundsPerShot;

    // Heat
    float HeatGeneratedPerRound;
    float AgeGeneratedPerRound;

    // Noise
    uint16_t FiringNoise; // 0=Silent..4=Loud

    // Accuracy penalties
    float ReloadPenalty;
    float SwitchPenalty;
};


struct WeaponMagazineData
{
    int16_t RoundsTotalInitial;
    int16_t RoundsTotalMaximum;
    int16_t RoundsLoadedMaximum;
    int16_t RoundsInventoryMaximum;
    int16_t RoundsReloaded;
};

struct WeaponTriggerData
{
    uint16_t Input;         // 0=RightTrigger, 1=LeftTrigger, 2=Melee
    uint16_t Behavior;      // 0=Spew, 1=Latch, 2=LatchAutofire, 4=Charge...
    float ChargingTime;
    float ChargedTime;
    float AutofireTime;
    bool IsChargeable;  // ChargingTime > 0
};

struct WeaponTargetTrackingData
{
    float AcquireTime;
    float GraceTime;
    float DecayTime;
    int32_t TrackingTypeCount;
};

struct WeaponData
{
    std::string TagName;

    // Classification
    uint16_t WeaponType;          // raw enum: Undefined=0, Shotgun=1...
    uint32_t WeaponClass;         // stringid raw
    uint32_t WeaponName;          // stringid raw
    int16_t MagnificationLevels;
    float MagnificationRangeMin;
    float MagnificationRangeMax;

    // Flags
    bool IsSupportWeapon;
    bool MustBeReadied;
    bool PreventsGrenade;
    bool PreventsMelee;
    bool PreventsCrouching;
    bool CannotFireWhileBoosting;
    bool AllowsBinoculars;
    bool Uses3rdPersonCamera;

    // Aim
    float AutoaimAngle;
    float AutoaimRange;
    float AutoaimFalloffRange;
    float MagnetismAngle;
    float MagnetismRange;
    float MagnetismFalloffRange;
    float MagnetismNearFalloffRange;
    float DeviationAngle;

    // Movement penalty
    float ForwardMovementPenalty;
    float SidewaysMovementPenalty;

    // Heat
    float OverheatedThreshold;
    float HeatRecoveryThreshold;
    float HeatLossPerSecond;
    float OverheatedHeatLossPerSecond;
    float HeatWarningThreshold;
    float HeatVentingTime;
    bool  CanOverheat;

    // AI
    float AiScariness;
    float ActiveCamoDing;
    float ReadyTime;
    float AgeMisfireStart;
    float AgeMisfireChance;

    // Primary barrel
    bool HasBarrel;
    WeaponBarrelData PrimaryBarrel;

    // Primary magazine
    bool HasMagazine;
    WeaponMagazineData PrimaryMagazine;

    // Primary trigger
    bool HasTrigger;
    WeaponTriggerData PrimaryTrigger;

    // Target tracking
    bool HasTargetTracking;
    WeaponTargetTrackingData TargetTracking;
};