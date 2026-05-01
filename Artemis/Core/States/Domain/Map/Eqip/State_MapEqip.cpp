#include "pch.h"
#include "Core/States/Domain/Map/Eqip/State_MapEqip.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

bool State_MapEqip::HasEqip(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Eqips.count(tagName) > 0;
}

const EqipObject* State_MapEqip::GetEqip(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Eqips.find(tagName);
    return it != m_Eqips.end() ? &it->second : nullptr;
}

void State_MapEqip::AddEqip(const std::string& tagName, EqipObject data)
{
    //this->LogRawData(tagName, data);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Eqips.emplace(tagName, std::move(data));
}

void State_MapEqip::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Eqips.clear();
}

void State_MapEqip::LogRawData(const std::string& tagName, const EqipObject& eqip)
{
    const auto& d = eqip.Data;

    g_pSystem->Debug->Log("=== EQIP RAW: %s ===", tagName.c_str());

    // --- Base ---
    g_pSystem->Debug->Log("  [Base]");
    g_pSystem->Debug->Log("    BoundingRadius:              %.3f", d.BoundingRadius);
    g_pSystem->Debug->Log("    Flags_3:                     0x%08X", d.Flags_3);
    g_pSystem->Debug->Log("      PathfindingObstacle:       %d", (d.Flags_3 >> 0) & 1);
    g_pSystem->Debug->Log("      DangerousToAI:             %d", (d.Flags_3 >> 1) & 1);
    g_pSystem->Debug->Log("      NeverDroppedByAI:          %d", (d.Flags_3 >> 2) & 1);
    g_pSystem->Debug->Log("      ProtectsParentFromAOE:     %d", (d.Flags_3 >> 3) & 1);
    g_pSystem->Debug->Log("      ThirdPersonWhileActive:    %d", (d.Flags_3 >> 4) & 1);
    g_pSystem->Debug->Log("      ThirdPersonAlways:         %d", (d.Flags_3 >> 5) & 1);
    g_pSystem->Debug->Log("      CannotBeActiveAirborne:    %d", (d.Flags_3 >> 7) & 1);
    g_pSystem->Debug->Log("      CannotActivateAirborne:    %d", (d.Flags_3 >> 8) & 1);
    g_pSystem->Debug->Log("      CannotActivateInVehicle:   %d", (d.Flags_3 >> 10) & 1);
    g_pSystem->Debug->Log("      DeactivatedByFiringWeapon: %d", (d.Flags_3 >> 11) & 1);
    g_pSystem->Debug->Log("      DeactivatedByMelee:        %d", (d.Flags_3 >> 12) & 1);
    g_pSystem->Debug->Log("      DeactivatedByGrenade:      %d", (d.Flags_3 >> 13) & 1);

    // --- Timing ---
    g_pSystem->Debug->Log("  [Timing]");
    g_pSystem->Debug->Log("    WarmupTime:                  %.3f", d.WarmupTime);
    g_pSystem->Debug->Log("    Duration:                    %.3f", d.Duration);
    g_pSystem->Debug->Log("    CooldownTime:                %.3f", d.CooldownTime);
    g_pSystem->Debug->Log("    PhantomVolumeActivationTime: %.3f", d.PhantomVolumeActivationTime);
    g_pSystem->Debug->Log("    EnergyRecoveryTime:          %.3f", d.EnergyRecoveryTime);
    g_pSystem->Debug->Log("    Charges:                     %d", (int32_t)d.Charges);
    g_pSystem->Debug->Log("    ActivationType:              %u", (uint32_t)d.ActivationType);

    // --- Energy ---
    g_pSystem->Debug->Log("  [Energy]");
    g_pSystem->Debug->Log("    MinimumActivationEnergy:     %.3f", d.MinimumActivationEnergy);
    g_pSystem->Debug->Log("    ActivationEnergyCost:        %.3f", d.ActivationEnergyCost);
    g_pSystem->Debug->Log("    DeactivationEnergyCost:      %.3f", d.DeactivationEnergyCost);
    g_pSystem->Debug->Log("    ActiveEnergyRate:            %.3f", d.ActiveEnergyRate);
    g_pSystem->Debug->Log("    RechargeRate:                %.3f", d.RechargeRate);
    g_pSystem->Debug->Log("    MovementSpeedDomain:         %.3f", d.MovementSpeedDomain);

    // --- AI ---
    g_pSystem->Debug->Log("  [AI]");
    g_pSystem->Debug->Log("    DangerRadius:                %.3f", d.DangerRadius);
    g_pSystem->Debug->Log("    MinimumDeploymentDistance:   %.3f", d.MinimumDeploymentDistance);
    g_pSystem->Debug->Log("    AwarenessTime:               %.3f", d.AwarenessTime);
    g_pSystem->Debug->Log("    ObjectNoiseAdjustment:       %d", (int32_t)d.ObjectNoiseAdjustment);

    // --- MultiplayerObject ---
    g_pSystem->Debug->Log("  [MultiplayerObject: %zu]", eqip.MultiplayerObject.size());
    for (size_t i = 0; i < eqip.MultiplayerObject.size(); ++i)
    {
        const auto& mp = eqip.MultiplayerObject[i];
        g_pSystem->Debug->Log("    [%zu] Type:%u GameEngineFlags:%u"
            " DefaultSpawnTime:%d DefaultAbandonmentTime:%d"
            " BoundaryShape:%u BoundaryWidthRadius:%.3f"
            " BoundaryBoxLength:%.3f BoundaryHeight:%.3f",
            i,
            (uint32_t)mp.Type,
            (uint32_t)mp.GameEngineFlags,
            (int32_t)mp.DefaultSpawnTime,
            (int32_t)mp.DefaultAbandonmentTime,
            (uint32_t)mp.BoundaryShape,
            mp.BoundaryWidthRadius,
            mp.BoundaryBoxLength,
            mp.BoundaryHeight);
    }

    // --- SubBlocks presence ---
    g_pSystem->Debug->Log("  [SubBlocks]");
    g_pSystem->Debug->Log("    OverrideCamera:              %zu", eqip.OverrideCamera.size());
    g_pSystem->Debug->Log("    MultiplayerPowerup:          %zu", eqip.MultiplayerPowerup.size());
    g_pSystem->Debug->Log("    Spawner:                     %zu", eqip.Spawner.size());
    g_pSystem->Debug->Log("    ProximityMine:               %zu", eqip.ProximityMine.size());
    g_pSystem->Debug->Log("    MotionTrackerNoise:          %zu", eqip.MotionTrackerNoise.size());
    g_pSystem->Debug->Log("    InvincibilityMode:           %zu", eqip.InvincibilityMode.size());
    g_pSystem->Debug->Log("    TreeOfLife:                  %zu", eqip.TreeOfLife.size());
    g_pSystem->Debug->Log("    Shapeshifter:                %zu", eqip.Shapeshifter.size());
    g_pSystem->Debug->Log("    RepulsorField:               %zu", eqip.RepulsorField.size());
    g_pSystem->Debug->Log("    Iwhbydaddy:                  %zu", eqip.Iwhbydaddy.size());
    g_pSystem->Debug->Log("    SuperJump:                   %zu", eqip.SuperJump.size());
    g_pSystem->Debug->Log("    AmmoPack:                    %zu", eqip.AmmoPack.size());
    g_pSystem->Debug->Log("    PowerFist:                   %zu", eqip.PowerFist.size());
    g_pSystem->Debug->Log("    HealthPack:                  %zu", eqip.HealthPack.size());
    g_pSystem->Debug->Log("    Jetpack:                     %zu", eqip.Jetpack.size());
    g_pSystem->Debug->Log("    Hologram:                    %zu", eqip.Hologram.size());
    g_pSystem->Debug->Log("    SpecialWeapon:               %zu", eqip.SpecialWeapon.size());
    g_pSystem->Debug->Log("    SpecialMove:                 %zu", eqip.SpecialMove.size());
    g_pSystem->Debug->Log("    EngineerShields:             %zu", eqip.EngineerShields.size());
    g_pSystem->Debug->Log("    Sprint:                      %zu", eqip.Sprint.size());

    // --- Spawner detail ---
    for (size_t i = 0; i < eqip.Spawner.size(); ++i)
    {
        const auto& s = eqip.Spawner[i];
        g_pSystem->Debug->Log("    Spawner[%zu] SpawnRadius:%.3f SpawnZOffset:%.3f"
            " SpawnAreaRadius:%.3f SpawnVelocity:%.3f Type:%u",
            i, s.SpawnRadius, s.SpawnZOffset,
            s.SpawnAreaRadius, s.SpawnVelocity,
            (uint32_t)s.Type);
    }

    // --- ProximityMine detail ---
    for (size_t i = 0; i < eqip.ProximityMine.size(); ++i)
    {
        const auto& pm = eqip.ProximityMine[i];
        g_pSystem->Debug->Log("    ProximityMine[%zu] ArmTime:%.3f SelfDestructTime:%.3f"
            " TriggerTime:%.3f TriggerVelocity:%.3f",
            i, pm.ArmTime, pm.SelfDestructTime,
            pm.TriggerTime, pm.TriggerVelocity);
    }

    // --- MotionTrackerNoise detail ---
    for (size_t i = 0; i < eqip.MotionTrackerNoise.size(); ++i)
    {
        const auto& mt = eqip.MotionTrackerNoise[i];
        g_pSystem->Debug->Log("    MotionTrackerNoise[%zu] ArmTime:%.3f NoiseRadius:%.3f"
            " NoiseCount:%d FlashRadius:%.3f",
            i, mt.ArmTime, mt.NoiseRadius,
            mt.NoiseCount, mt.FlashRadius);
    }

    // --- RepulsorField detail ---
    for (size_t i = 0; i < eqip.RepulsorField.size(); ++i)
    {
        const auto& rf = eqip.RepulsorField[i];
        g_pSystem->Debug->Log("    RepulsorField[%zu] Flags:0x%08X Radius:%.3f Power:%.3f",
            i, rf.Flags, rf.Radius, rf.Power);
    }

    // --- Jetpack detail ---
    for (size_t i = 0; i < eqip.Jetpack.size(); ++i)
    {
        const auto& jp = eqip.Jetpack[i];
        g_pSystem->Debug->Log("    Jetpack[%zu] ThrustAttackTime:%.3f ThrustDecayTime:%.3f"
            " NegVelAccelScale:%.3f AirborneAccelScale:%.3f"
            " GroundedAccelMagnitude:%.3f LandingAnticipationTime:%.3f",
            i, jp.ThrustAttackTime, jp.ThrustDecayTime,
            jp.NegativeVelocityAccelerationScale,
            jp.AirborneAccelerationScale,
            jp.GroundedAccelerationMagnitude,
            jp.LandingAnticipationTime);
    }

    // --- Hologram detail ---
    for (size_t i = 0; i < eqip.Hologram.size(); ++i)
    {
        const auto& h = eqip.Hologram[i];
        g_pSystem->Debug->Log("    Hologram[%zu] Duration:%.3f ShimmerDecreaseRate:%.3f"
            " ShimmerBulletPing:%.3f",
            i, h.HologramDuration,
            h.ShimmerDecreaseRate,
            h.ShimmerBulletPing);
    }

    // --- InvincibilityMode detail ---
    for (size_t i = 0; i < eqip.InvincibilityMode.size(); ++i)
    {
        const auto& inv = eqip.InvincibilityMode[i];
        g_pSystem->Debug->Log("    InvincibilityMode[%zu] ShieldRechargeRate:%.3f"
            " ShieldMaxRechargeLevel:%.3f MaxVerticalVelocity:%.3f",
            i, inv.ShieldRechargeRate,
            inv.ShieldMaximumRechargeLevel,
            inv.MaximumVerticalVelocity);
    }

    // --- EngineerShields detail ---
    for (size_t i = 0; i < eqip.EngineerShields.size(); ++i)
    {
        const auto& es = eqip.EngineerShields[i];
        g_pSystem->Debug->Log("    EngineerShields[%zu] Radius:%.3f Flags:0x%08X",
            i, es.Radius, es.Flags);
    }

    // --- AmmoPack detail ---
    for (size_t i = 0; i < eqip.AmmoPack.size(); ++i)
    {
        const auto& ap = eqip.AmmoPack[i];
        g_pSystem->Debug->Log("    AmmoPack[%zu] EnergyChangePerClip:%.3f"
            " ExtraFrags:%d ExtraPlasma:%d",
            i, ap.EnergyChangePerClipAdded,
            ap.ExtraFrags, ap.ExtraPlasma);
    }

    g_pSystem->Debug->Log("=== END EQIP: %s ===", tagName.c_str());
}