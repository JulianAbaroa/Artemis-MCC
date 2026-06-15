#include "pch.h"

#include "System_WeapBuilder.h"

#include "Core/Types/Sources/Static/Stats/Weap.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include "Generated/Weap/WeapObject.h"

Weap System_WeapBuilder::Build(const WeapObject& weap)
{
    Weap out{};
    const auto& data = weap.Data;

    out.TagName = weap.TagName;

    out.WeaponType = data.WeaponType;
    out.WeaponClass = data.WeaponClass;
    out.WeaponName = data.WeaponName;
    out.MagnificationLevels = data.MagnificationLevels;
    out.MagnificationRangeMin = data.MagnificationRange.Min;
    out.MagnificationRangeMax = data.MagnificationRange.Max;

    out.IsSupportWeapon = (data.Flags_3 & (1u << 8)) != 0;
    out.MustBeReadied = (data.Flags_3 & (1u << 0)) != 0;
    out.PreventsGrenade = (data.Flags_3 & (1u << 3)) != 0;
    out.PreventsMelee = (data.Flags_3 & (1u << 4)) != 0;
    out.PreventsCrouching = (data.Flags_3 & (1u << 12)) != 0;
    out.CannotFireWhileBoosting = (data.Flags_3 & (1u << 13)) != 0;
    out.AllowsBinoculars = (data.Flags_3 & (1u << 10)) != 0;
    out.Uses3rdPersonCamera = (data.Flags_3 & (1u << 15)) != 0;

    out.AutoaimAngle = data.AutoaimAngle;
    out.AutoaimRange = data.AutoaimRange;
    out.AutoaimFalloffRange = data.AutoaimFalloffRange;
    out.MagnetismAngle = data.MagnetismAngle;
    out.MagnetismRange = data.MagnetismRange;
    out.MagnetismFalloffRange = data.MagnetismFalloffRange;
    out.MagnetismNearFalloffRange = data.MagnetismNearFalloffRange;
    out.DeviationAngle = data.DeviationAngle;

    out.ForwardMovementPenalty = data.ForwardMovementPenalty;
    out.SidewaysMovementPenalty = data.SidewaysMovementPenalty;

    out.OverheatedThreshold = data.OverheatedThreshold;
    out.HeatRecoveryThreshold = data.HeatRecoveryThreshold;
    out.HeatLossPerSecond = data.HeatLossPerSecond;
    out.OverheatedHeatLossPerSecond = data.OverheatedHeatLossPerSecond;
    out.HeatWarningThreshold = data.HeatWarningThreshold;
    out.HeatVentingTime = data.HeatVentingTime;
    out.CanOverheat = data.OverheatedThreshold > 0.0f;

    out.AiScariness = data.AiScariness;
    out.ActiveCamoDing = data.ActiveCamoDing;
    out.ReadyTime = data.ReadyTime;
    out.AgeMisfireStart = data.AgeMisfireStart;
    out.AgeMisfireChance = data.AgeMisfireChance;

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

WeaponBarrelData System_WeapBuilder::BuildBarrel(
    const WeapObject& weap)
{
    WeaponBarrelData out{};
    if (weap.Barrels.empty()) return out;

    const auto& barrel = weap.Barrels[0];

    out.RoundsPerSecondMin = barrel.RoundsPerSecond.Min;
    out.RoundsPerSecondMax = barrel.RoundsPerSecond.Max;
    out.AccelerationTime = barrel.AccelerationTime;
    out.DecelerationTime = barrel.DecelerationTime;
    out.FireRecoveryTime = barrel.FireRecoveryTime;
    out.MinimumError = barrel.MinimumError;
    out.ErrorAngleMin = barrel.ErrorAngle.Min;
    out.ErrorAngleMax = barrel.ErrorAngle.Max;
    out.RoundsPerShot = barrel.RoundsPerShot;
    out.HeatGeneratedPerRound = barrel.HeatGeneratedPerRound;
    out.AgeGeneratedPerRound = barrel.AgeGeneratedPerRound;
    out.FiringNoise = barrel.FiringNoise;
    out.ReloadPenalty = barrel.ReloadPenalty;
    out.SwitchPenalty = barrel.SwitchPenalty;

    return out;
}

WeaponMagazineData System_WeapBuilder::BuildMagazine(
    const WeapObject& weap)
{
    WeaponMagazineData out{};
    if (weap.Magazines.empty()) return out;

    const auto& magazine = weap.Magazines[0];
    out.RoundsTotalInitial = magazine.RoundsTotalInitial;
    out.RoundsTotalMaximum = magazine.RoundsTotalMaximum;
    out.RoundsLoadedMaximum = magazine.RoundsLoadedMaximum;
    out.RoundsInventoryMaximum = magazine.RoundsInventoryMaximum;
    out.RoundsReloaded = magazine.RoundsReloaded;

    return out;
}

WeaponTriggerData System_WeapBuilder::BuildTrigger(
    const WeapObject& weap)
{
    WeaponTriggerData out{};
    if (weap.NewTriggers.empty()) return out;

    const auto& trigger = weap.NewTriggers[0];
    out.Input = trigger.Input;
    out.Behavior = trigger.Behavior;
    out.ChargingTime = trigger.ChargingTime;
    out.ChargedTime = trigger.ChargedTime;
    out.AutofireTime = trigger.AutofireTime;
    out.IsChargeable = trigger.ChargingTime > 0.0f;

    return out;
}

WeaponTargetTrackingData System_WeapBuilder::BuildTargetTracking(
    const WeapObject& weap)
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