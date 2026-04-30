#include "pch.h"
#include "Core/Systems/Domain/Interactable/Weap/System_WeapDataBuilder.h"
#include "Core/Types/Domain/Domains/Interactable/WeaponData.h"
#include "Generated/Weap/WeapObject.h"

WeaponData System_WeapDataBuilder::BuildData(const WeapObject& weap)
{
    WeaponData out{};

    out.TagName = weap.TagName;

    // Classification.
    out.WeaponType = weap.Data.WeaponType;
    out.WeaponClass = weap.Data.WeaponClass;
    out.WeaponName = weap.Data.WeaponName;
    out.MagnificationLevels = weap.Data.MagnificationLevels;
    out.HasTargetTracking = !weap.TargetTracking.empty();

    // Flags_3 bit 8 = Support Weapon
    out.IsSupportWeapon = (weap.Data.Flags_3 & (1u << 8)) != 0;

    // Movement.
    out.ForwardMovementPenalty = weap.Data.ForwardMovementPenalty;
    out.SidewaysMovementPenalty = weap.Data.SidewaysMovementPenalty;

    // Autoaim
    out.AutoaimAngle = weap.Data.AutoaimAngle;
    out.AutoaimRange = weap.Data.AutoaimRange;

    // Heat.
    out.OverheatedThreshold = weap.Data.OverheatedThreshold;
    out.HeatLossPerSecond = weap.Data.HeatLossPerSecond;
    out.CanOverheat = weap.Data.OverheatedThreshold > 0.0f;

    // AI
    out.AiScariness = weap.Data.AiScariness;

    // Primary barrel.
    out.HasBarrel = !weap.Barrels.empty();
    out.PrimaryBarrel = BuildBarrel(weap);

    // Primary Magazine.
    out.HasMagazine = !weap.Magazines.empty();
    out.PrimaryMagazine = BuildMagazine(weap);

    return out;
}


WeaponBarrelData System_WeapDataBuilder::BuildBarrel(const WeapObject& weap)
{
    WeaponBarrelData out{};
    if (weap.Barrels.empty()) return out;

    const auto& b = weap.Barrels[0];

    // RoundsPerSecond is a range16 packed in uint32
    // bits[31:16] = max, bits[15:0] = min, both normalized int16
    // In WeapData it's directly as RangeF
    out.RoundsPerSecondMin = b.RoundsPerSecond.Min;
    out.RoundsPerSecondMax = b.RoundsPerSecond.Max;

    out.MinimumError = b.MinimumError;
    out.ErrorAngleMin = b.ErrorAngle.Min;
    out.ErrorAngleMax = b.ErrorAngle.Max;

    out.ProjectilesPerShot = b.ProjectilesPerShot;
    out.FiringNoise = b.FiringNoise;

    return out;
}

WeaponMagazineData System_WeapDataBuilder::BuildMagazine(const WeapObject& weap)
{
    WeaponMagazineData out{};
    if (weap.Magazines.empty()) return out;

    const auto& m = weap.Magazines[0];
    out.RoundsTotalMaximum = m.RoundsTotalMaximum;
    out.RoundsLoadedMaximum = m.RoundsLoadedMaximum;

    return out;
}