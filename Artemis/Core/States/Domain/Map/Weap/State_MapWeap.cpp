#include "pch.h"
#include "Core/States/Domain/Map/Weap/State_MapWeap.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

bool State_MapWeap::HasWeap(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Weaps.count(tagName) > 0;
}

const WeapObject* State_MapWeap::GetWeap(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Weaps.find(tagName);
    return it != m_Weaps.end() ? &it->second : nullptr;
}

void State_MapWeap::AddWeap(const std::string& tagName, WeapObject data)
{
    //this->LogRawData(tagName, data);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Weaps.emplace(tagName, std::move(data));
}

void State_MapWeap::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Weaps.clear();
}

void State_MapWeap::LogRawData(const std::string& tagName, const WeapObject& weap)
{
    const auto& d = weap.Data;

    g_pSystem->Debug->Log("=== WEAP RAW: %s ===", tagName.c_str());

    // --- Object Base ---
    g_pSystem->Debug->Log("  [Base]");
    g_pSystem->Debug->Log("    BoundingRadius:              %.3f", d.BoundingRadius);
    g_pSystem->Debug->Log("    WeaponType:                  %u", (uint32_t)d.WeaponType);
    g_pSystem->Debug->Log("    WeaponClass:                 %u", d.WeaponClass);
    g_pSystem->Debug->Log("    WeaponName:                  %u", d.WeaponName);
    g_pSystem->Debug->Log("    MagnificationLevels:         %d", (int32_t)d.MagnificationLevels);
    g_pSystem->Debug->Log("    MagnificationRange.Min:      %.3f", d.MagnificationRange.Min);
    g_pSystem->Debug->Log("    MagnificationRange.Max:      %.3f", d.MagnificationRange.Max);
    g_pSystem->Debug->Log("    Flags_3:                     0x%08X", d.Flags_3);
    g_pSystem->Debug->Log("      MustBeReadied:             %d", (d.Flags_3 >> 0) & 1);
    g_pSystem->Debug->Log("      AimAssistsOnlyWhenZoomed:  %d", (d.Flags_3 >> 2) & 1);
    g_pSystem->Debug->Log("      PreventsGrenadeThrow:      %d", (d.Flags_3 >> 3) & 1);
    g_pSystem->Debug->Log("      PreventsMeleeAttack:       %d", (d.Flags_3 >> 4) & 1);
    g_pSystem->Debug->Log("      SupportWeapon:             %d", (d.Flags_3 >> 8) & 1);
    g_pSystem->Debug->Log("      AllowsBinoculars:          %d", (d.Flags_3 >> 10) & 1);
    g_pSystem->Debug->Log("      PreventsCrouching:         %d", (d.Flags_3 >> 12) & 1);
    g_pSystem->Debug->Log("      CannotFireWhileBoosting:   %d", (d.Flags_3 >> 13) & 1);
    g_pSystem->Debug->Log("      Uses3rdPersonCamera:       %d", (d.Flags_3 >> 15) & 1);

    // --- Aim / Autoaim ---
    g_pSystem->Debug->Log("  [Aim]");
    g_pSystem->Debug->Log("    AutoaimAngle:                %.3f", d.AutoaimAngle);
    g_pSystem->Debug->Log("    AutoaimRange:                %.3f", d.AutoaimRange);
    g_pSystem->Debug->Log("    AutoaimFalloffRange:         %.3f", d.AutoaimFalloffRange);
    g_pSystem->Debug->Log("    AutoaimNearFalloffRange:     %.3f", d.AutoaimNearFalloffRange);
    g_pSystem->Debug->Log("    MagnetismAngle:              %.3f", d.MagnetismAngle);
    g_pSystem->Debug->Log("    MagnetismRange:              %.3f", d.MagnetismRange);
    g_pSystem->Debug->Log("    MagnetismFalloffRange:       %.3f", d.MagnetismFalloffRange);
    g_pSystem->Debug->Log("    MagnetismNearFalloffRange:   %.3f", d.MagnetismNearFalloffRange);
    g_pSystem->Debug->Log("    DeviationAngle:              %.3f", d.DeviationAngle);

    // --- Movement ---
    g_pSystem->Debug->Log("  [Movement]");
    g_pSystem->Debug->Log("    MovementPenalized:           %u", (uint32_t)d.MovementPenalized);
    g_pSystem->Debug->Log("    ForwardMovementPenalty:      %.3f", d.ForwardMovementPenalty);
    g_pSystem->Debug->Log("    SidewaysMovementPenalty:     %.3f", d.SidewaysMovementPenalty);

    // --- Heat ---
    g_pSystem->Debug->Log("  [Heat]");
    g_pSystem->Debug->Log("    HeatRecoveryThreshold:       %.3f", d.HeatRecoveryThreshold);
    g_pSystem->Debug->Log("    OverheatedThreshold:         %.3f", d.OverheatedThreshold);
    g_pSystem->Debug->Log("    HeatDetonationThreshold:     %.3f", d.HeatDetonationThreshold);
    g_pSystem->Debug->Log("    HeatDetonationFraction:      %.3f", d.HeatDetonationFraction);
    g_pSystem->Debug->Log("    HeatLossPerSecond:           %.3f", d.HeatLossPerSecond);
    g_pSystem->Debug->Log("    HeatVentingTime:             %.3f", d.HeatVentingTime);
    g_pSystem->Debug->Log("    HeatVentingExitHeat:         %.3f", d.HeatVentingExitHeat);
    g_pSystem->Debug->Log("    HeatIllumination:            %.3f", d.HeatIllumination);
    g_pSystem->Debug->Log("    HeatWarningThreshold:        %.3f", d.HeatWarningThreshold);
    g_pSystem->Debug->Log("    OverheatedHeatLossPerSecond: %.3f", d.OverheatedHeatLossPerSecond);

    // --- AI ---
    g_pSystem->Debug->Log("  [AI]");
    g_pSystem->Debug->Log("    AiScariness:                 %.3f", d.AiScariness);
    g_pSystem->Debug->Log("    ActiveCamoDing:              %.3f", d.ActiveCamoDing);
    g_pSystem->Debug->Log("    ReadyTime:                   %.3f", d.ReadyTime);
    g_pSystem->Debug->Log("    TurnOnTime:                  %.3f", d.TurnOnTime);
    g_pSystem->Debug->Log("    AgeHeatRecoveryPenalty:      %.3f", d.AgeHeatRecoveryPenalty);
    g_pSystem->Debug->Log("    AgeRateOfFirePenalty:        %.3f", d.AgeRateOfFirePenalty);
    g_pSystem->Debug->Log("    AgeMisfireStart:             %.3f", d.AgeMisfireStart);
    g_pSystem->Debug->Log("    AgeMisfireChance:            %.3f", d.AgeMisfireChance);

    // --- Ballistic Arc ---
    g_pSystem->Debug->Log("  [BallisticArc]");
    g_pSystem->Debug->Log("    FiringBounds.Min:            %.3f", d.BallisticArcingFiringBounds.Min);
    g_pSystem->Debug->Log("    FiringBounds.Max:            %.3f", d.BallisticArcingFiringBounds.Max);
    g_pSystem->Debug->Log("    FractionBounds.Min:          %.3f", d.BallisticArcingFractionBounds.Min);
    g_pSystem->Debug->Log("    FractionBounds.Max:          %.3f", d.BallisticArcingFractionBounds.Max);

    // --- TargetTracking ---
    g_pSystem->Debug->Log("  [TargetTracking: %zu]", weap.TargetTracking.size());
    for (size_t i = 0; i < weap.TargetTracking.size(); ++i)
    {
        const auto& t = weap.TargetTracking[i];
        g_pSystem->Debug->Log("    [%zu] AcquireTime:%.3f GraceTime:%.3f DecayTime:%.3f TrackingTypes:%zu",
            i, t.AcquireTime, t.GraceTime, t.DecayTime, t.TrackingTypes.size());
    }

    // --- Magazines ---
    g_pSystem->Debug->Log("  [Magazines: %zu]", weap.Magazines.size());
    for (size_t i = 0; i < weap.Magazines.size(); ++i)
    {
        const auto& m = weap.Magazines[i];
        g_pSystem->Debug->Log("    [%zu] RoundsRecharged:%d RoundsTotalInitial:%d"
            " RoundsTotalMaximum:%d RoundsLoadedMaximum:%d"
            " RoundsInventoryMaximum:%d RoundsReloaded:%d",
            i,
            (int32_t)m.RoundsRecharged,
            (int32_t)m.RoundsTotalInitial,
            (int32_t)m.RoundsTotalMaximum,
            (int32_t)m.RoundsLoadedMaximum,
            (int32_t)m.RoundsInventoryMaximum,
            (int32_t)m.RoundsReloaded);
    }

    // --- NewTriggers ---
    g_pSystem->Debug->Log("  [NewTriggers: %zu]", weap.NewTriggers.size());
    for (size_t i = 0; i < weap.NewTriggers.size(); ++i)
    {
        const auto& t = weap.NewTriggers[i];
        g_pSystem->Debug->Log("    [%zu] Input:%u Behavior:%u ChargingTime:%.3f"
            " ChargedTime:%.3f AutofireTime:%.3f",
            i,
            (uint32_t)t.Input,
            (uint32_t)t.Behavior,
            t.ChargingTime,
            t.ChargedTime,
            t.AutofireTime);
    }

    // --- Barrels ---
    g_pSystem->Debug->Log("  [Barrels: %zu]", weap.Barrels.size());
    for (size_t i = 0; i < weap.Barrels.size(); ++i)
    {
        const auto& b = weap.Barrels[i];
        g_pSystem->Debug->Log("    [%zu] RoundsPerSecond:[%.3f,%.3f]"
            " ProjectilesPerShot:%d RoundsPerShot:%d"
            " MinimumRoundsLoaded:%d FiringNoise:%u"
            " MinimumError:%.3f ErrorAngle:[%.3f,%.3f]"
            " FireRecoveryTime:%.3f AccelerationTime:%.3f"
            " DecelerationTime:%.3f HeatGeneratedPerRound:%.3f"
            " AgeGeneratedPerRound:%.3f ReloadPenalty:%.3f"
            " SwitchPenalty:%.3f ZoomPenalty:%.3f JumpPenalty:%.3f",
            i,
            b.RoundsPerSecond.Min, b.RoundsPerSecond.Max,
            (int32_t)b.ProjectilesPerShot,
            (int32_t)b.RoundsPerShot,
            (int32_t)b.MinimumRoundsLoaded,
            (uint32_t)b.FiringNoise,
            b.MinimumError,
            b.ErrorAngle.Min, b.ErrorAngle.Max,
            b.FireRecoveryTime,
            b.AccelerationTime,
            b.DecelerationTime,
            b.HeatGeneratedPerRound,
            b.AgeGeneratedPerRound,
            b.ReloadPenalty,
            b.SwitchPenalty,
            b.ZoomPenalty,
            b.JumpPenalty);
    }

    g_pSystem->Debug->Log("=== END WEAP: %s ===", tagName.c_str());
}