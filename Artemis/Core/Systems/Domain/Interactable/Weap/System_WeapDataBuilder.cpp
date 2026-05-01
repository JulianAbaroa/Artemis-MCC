#include "pch.h"
#include "Core/Systems/Domain/Interactable/Weap/System_WeapDataBuilder.h"
#include "Core/Types/Domain/Domains/Interactable/WeaponData.h"
#include "Generated/Weap/WeapObject.h"

WeaponData System_WeapDataBuilder::BuildData(const WeapObject& weap)
{
    WeaponData out{};
    const auto& d = weap.Data;

    out.TagName = weap.TagName;

    // Classification
    out.WeaponType = d.WeaponType;
    out.WeaponClass = d.WeaponClass;
    out.WeaponName = d.WeaponName;
    out.MagnificationLevels = d.MagnificationLevels;
    out.MagnificationRangeMin = d.MagnificationRange.Min;
    out.MagnificationRangeMax = d.MagnificationRange.Max;

    // Flags
    out.IsSupportWeapon = (d.Flags_3 & (1u << 8)) != 0;
    out.MustBeReadied = (d.Flags_3 & (1u << 0)) != 0;
    out.PreventsGrenade = (d.Flags_3 & (1u << 3)) != 0;
    out.PreventsMelee = (d.Flags_3 & (1u << 4)) != 0;
    out.PreventsCrouching = (d.Flags_3 & (1u << 12)) != 0;
    out.CannotFireWhileBoosting = (d.Flags_3 & (1u << 13)) != 0;
    out.AllowsBinoculars = (d.Flags_3 & (1u << 10)) != 0;
    out.Uses3rdPersonCamera = (d.Flags_3 & (1u << 15)) != 0;

    // Aim
    out.AutoaimAngle = d.AutoaimAngle;
    out.AutoaimRange = d.AutoaimRange;
    out.AutoaimFalloffRange = d.AutoaimFalloffRange;
    out.MagnetismAngle = d.MagnetismAngle;
    out.MagnetismRange = d.MagnetismRange;
    out.MagnetismFalloffRange = d.MagnetismFalloffRange;
    out.MagnetismNearFalloffRange = d.MagnetismNearFalloffRange;
    out.DeviationAngle = d.DeviationAngle;

    // Movement
    out.ForwardMovementPenalty = d.ForwardMovementPenalty;
    out.SidewaysMovementPenalty = d.SidewaysMovementPenalty;

    // Heat
    out.OverheatedThreshold = d.OverheatedThreshold;
    out.HeatRecoveryThreshold = d.HeatRecoveryThreshold;
    out.HeatLossPerSecond = d.HeatLossPerSecond;
    out.OverheatedHeatLossPerSecond = d.OverheatedHeatLossPerSecond;
    out.HeatWarningThreshold = d.HeatWarningThreshold;
    out.HeatVentingTime = d.HeatVentingTime;
    out.CanOverheat = d.OverheatedThreshold > 0.0f;

    // AI
    out.AiScariness = d.AiScariness;
    out.ActiveCamoDing = d.ActiveCamoDing;
    out.ReadyTime = d.ReadyTime;
    out.AgeMisfireStart = d.AgeMisfireStart;
    out.AgeMisfireChance = d.AgeMisfireChance;

    // Sub-structs
    out.HasBarrel = !weap.Barrels.empty();
    out.PrimaryBarrel = BuildBarrel(weap);

    out.HasMagazine = !weap.Magazines.empty();
    out.PrimaryMagazine = BuildMagazine(weap);

    out.HasTrigger = !weap.NewTriggers.empty();
    out.PrimaryTrigger = BuildTrigger(weap);

    out.HasTargetTracking = !weap.TargetTracking.empty();
    out.TargetTracking = BuildTargetTracking(weap);

    return out;
}

WeaponBarrelData System_WeapDataBuilder::BuildBarrel(const WeapObject& weap)
{
    WeaponBarrelData out{};
    if (weap.Barrels.empty()) return out;

    const auto& b = weap.Barrels[0];

    out.RoundsPerSecondMin = b.RoundsPerSecond.Min;
    out.RoundsPerSecondMax = b.RoundsPerSecond.Max;
    out.AccelerationTime = b.AccelerationTime;
    out.DecelerationTime = b.DecelerationTime;
    out.FireRecoveryTime = b.FireRecoveryTime;
    out.MinimumError = b.MinimumError;
    out.ErrorAngleMin = b.ErrorAngle.Min;
    out.ErrorAngleMax = b.ErrorAngle.Max;
    out.RoundsPerShot = b.RoundsPerShot;
    out.HeatGeneratedPerRound = b.HeatGeneratedPerRound;
    out.AgeGeneratedPerRound = b.AgeGeneratedPerRound;
    out.FiringNoise = b.FiringNoise;
    out.ReloadPenalty = b.ReloadPenalty;
    out.SwitchPenalty = b.SwitchPenalty;

    return out;
}

WeaponMagazineData System_WeapDataBuilder::BuildMagazine(const WeapObject& weap)
{
    WeaponMagazineData out{};
    if (weap.Magazines.empty()) return out;

    const auto& m = weap.Magazines[0];
    out.RoundsTotalInitial = m.RoundsTotalInitial;
    out.RoundsTotalMaximum = m.RoundsTotalMaximum;
    out.RoundsLoadedMaximum = m.RoundsLoadedMaximum;
    out.RoundsInventoryMaximum = m.RoundsInventoryMaximum;
    out.RoundsReloaded = m.RoundsReloaded;

    return out;
}

WeaponTriggerData System_WeapDataBuilder::BuildTrigger(const WeapObject& weap)
{
    WeaponTriggerData out{};
    if (weap.NewTriggers.empty()) return out;

    const auto& t = weap.NewTriggers[0];
    out.Input = t.Input;
    out.Behavior = t.Behavior;
    out.ChargingTime = t.ChargingTime;
    out.ChargedTime = t.ChargedTime;
    out.AutofireTime = t.AutofireTime;
    out.IsChargeable = t.ChargingTime > 0.0f;

    return out;
}

WeaponTargetTrackingData System_WeapDataBuilder::BuildTargetTracking(const WeapObject& weap)
{
    WeaponTargetTrackingData out{};
    if (weap.TargetTracking.empty()) return out;

    const auto& t = weap.TargetTracking[0];
    out.AcquireTime = t.AcquireTime;
    out.GraceTime = t.GraceTime;
    out.DecayTime = t.DecayTime;
    out.TrackingTypeCount = static_cast<int32_t>(t.TrackingTypes.size());

    return out;
}