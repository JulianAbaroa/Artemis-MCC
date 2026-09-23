export module Resolved.Stats.Type:Weap;

import std;

export namespace Resolved::Stats::Type::Weap
{
    struct BarrelData
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
        std::int16_t RoundsPerShot;

        // Heat
        float HeatGeneratedPerRound;
        float AgeGeneratedPerRound;

        // Noise
        std::uint16_t FiringNoise; // 0=Silent..4=Loud

        // Accuracy penalties
        float ReloadPenalty;
        float SwitchPenalty;
    };

    struct MagazineData
    {
        std::int16_t RoundsTotalInitial;
        std::int16_t RoundsTotalMaximum;
        std::int16_t RoundsLoadedMaximum;
        std::int16_t RoundsInventoryMaximum;
        std::int16_t RoundsReloaded;
    };

    struct TriggerData
    {
        std::uint16_t Input;         // 0=RightTrigger, 1=LeftTrigger, 2=Melee
        std::uint16_t Behavior;      // 0=Spew, 1=Latch, 2=LatchAutofire, 4=Charge...
        float ChargingTime;
        float ChargedTime;
        float AutofireTime;
        bool IsChargeable;  // ChargingTime > 0
    };

    struct TargetTrackingData
    {
        float AcquireTime;
        float GraceTime;
        float DecayTime;
        std::int32_t TrackingTypeCount;
    };

    struct Weap
    {
        std::string TagName;

        // Classification
        std::uint16_t WeaponType;          // raw enum: Undefined=0, Shotgun=1...
        std::uint32_t WeaponClass;         // stringid raw
        std::uint32_t WeaponName;          // stringid raw
        std::int16_t MagnificationLevels;
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
        BarrelData PrimaryBarrel;

        // Primary magazine
        bool HasMagazine;
        MagazineData PrimaryMagazine;

        // Primary trigger
        bool HasTrigger;
        TriggerData PrimaryTrigger;

        // Target tracking
        bool HasTargetTracking;
        TargetTrackingData TargetTracking;
    };
}