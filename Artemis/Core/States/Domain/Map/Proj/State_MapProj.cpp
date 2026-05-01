#include "pch.h"
#include "Core/States/Domain/Map/Proj/State_MapProj.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

bool State_MapProj::HasProj(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Projs.count(tagName) > 0;
}

const ProjObject* State_MapProj::GetProj(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Projs.find(tagName);
    return it != m_Projs.end() ? &it->second : nullptr;
}

void State_MapProj::AddProj(const std::string& tagName, ProjObject data)
{
    //this->LogRawData(tagName, data);

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Projs.emplace(tagName, std::move(data));
}

void State_MapProj::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Projs.clear();
}

void State_MapProj::LogRawData(const std::string& tagName, const ProjObject& proj)
{
    const auto& d = proj.Data;

    g_pSystem->Debug->Log("=== PROJ RAW: %s ===", tagName.c_str());

    // --- Kinematics ---
    g_pSystem->Debug->Log("  [Kinematics]");
    g_pSystem->Debug->Log("    InitialVelocity:             %.3f", d.InitialVelocity);
    g_pSystem->Debug->Log("    FinalVelocity:               %.3f", d.FinalVelocity);
    g_pSystem->Debug->Log("    IndirectFireVelocity:        %.3f", d.IndirectFireVelocity);
    g_pSystem->Debug->Log("    AirGravityScale:             %.3f", d.AirGravityScale);
    g_pSystem->Debug->Log("    WaterGravityScale:           %.3f", d.WaterGravityScale);
    g_pSystem->Debug->Log("    AccelerationRange.Min:       %.3f", d.AccelerationRange.Min);
    g_pSystem->Debug->Log("    AccelerationRange.Max:       %.3f", d.AccelerationRange.Max);
    g_pSystem->Debug->Log("    AirDamageRange.Min:          %.3f", d.AirDamageRange.Min);
    g_pSystem->Debug->Log("    AirDamageRange.Max:          %.3f", d.AirDamageRange.Max);
    g_pSystem->Debug->Log("    WaterDamageRange.Min:        %.3f", d.WaterDamageRange.Min);
    g_pSystem->Debug->Log("    WaterDamageRange.Max:        %.3f", d.WaterDamageRange.Max);
    g_pSystem->Debug->Log("    RuntimeAccelerationBound:    %.3f", d.RuntimeAccelerationBoundInverse);

    // --- Guidance ---
    g_pSystem->Debug->Log("  [Guidance]");
    g_pSystem->Debug->Log("    GuidedAngVelLower:           %.3f", d.GuidedAngularVelocityLower);
    g_pSystem->Debug->Log("    GuidedAngVelUpper:           %.3f", d.GuidedAngularVelocityUpper);
    g_pSystem->Debug->Log("    GuidedAngVelAtRest:          %.3f", d.GuidedAngularVelocityAtRest);
    g_pSystem->Debug->Log("    TargetedLeadingFraction:     %.3f", d.TargetedLeadingFraction);
    g_pSystem->Debug->Log("    OuterRangeErrorRadius:       %.3f", d.GuidedProjectileOuterRangeErrorRadius);
    g_pSystem->Debug->Log("    AutoaimLeadingMaxLeadTime:   %.3f", d.AutoaimLeadingMaximumLeadTime);
    g_pSystem->Debug->Log("    AiNormalVelocityScale:       %.3f", d.AiNormalVelocityScale);
    g_pSystem->Debug->Log("    AiHeroicVelocityScale:       %.3f", d.AiHeroicVelocityScale);
    g_pSystem->Debug->Log("    AiLegendaryVelocityScale:    %.3f", d.AiLegendaryVelocityScale);
    g_pSystem->Debug->Log("    AiNormalGuidedAngVelScale:   %.3f", d.AiNormalGuidedAngularVelocityScale);
    g_pSystem->Debug->Log("    AiLegendaryGuidedAngVelScale:%.3f", d.AiLegendaryGuidedAngularVelocityScale);

    // --- Detonation ---
    g_pSystem->Debug->Log("  [Detonation]");
    g_pSystem->Debug->Log("    CollisionRadius:             %.3f", d.CollisionRadius);
    g_pSystem->Debug->Log("    ArmingTime:                  %.3f", d.ArmingTime);
    g_pSystem->Debug->Log("    Timer.Min:                   %.3f", d.Timer.Min);
    g_pSystem->Debug->Log("    Timer.Max:                   %.3f", d.Timer.Max);
    g_pSystem->Debug->Log("    MinimumVelocity:             %.3f", d.MinimumVelocity);
    g_pSystem->Debug->Log("    MaximumRange:                %.3f", d.MaximumRange);
    g_pSystem->Debug->Log("    BounceMaximumRange:          %.3f", d.BounceMaximumRange);
    g_pSystem->Debug->Log("    DetonationTimerStarts:       %u", (uint32_t)d.DetonationTimerStarts);
    g_pSystem->Debug->Log("    SuperDetonationTime:         %.3f", d.SuperDetonationTime);
    g_pSystem->Debug->Log("    SuperDetonationProjCount:    %d", d.SuperDetonationProjectileCount);
    g_pSystem->Debug->Log("    BoardingDetonationTime:      %.3f", d.BoardingDetonationTime);
    g_pSystem->Debug->Log("    MaxLatchTimeToDetonate:      %.3f", d.MaximumLatchTimeToDetonate);
    g_pSystem->Debug->Log("    MaxLatchTimeToArm:           %.3f", d.MaximumLatchTimeToArm);

    // --- Danger ---
    g_pSystem->Debug->Log("  [Danger]");
    g_pSystem->Debug->Log("    DangerRadius:                %.3f", d.DangerRadius);
    g_pSystem->Debug->Log("    DangerStimuliRadius:         %.3f", d.DangerStimuliRadius);
    g_pSystem->Debug->Log("    DangerGroupBurstCount:       %d", d.DangerGroupBurstCount);
    g_pSystem->Debug->Log("    DangerGroupBurstMaxCount:    %d", d.DangerGroupBurstMaximumCount);
    g_pSystem->Debug->Log("    MaterialEffectRadius:        %.3f", d.MaterialEffectRadius);
    g_pSystem->Debug->Log("    FlybyDamageMaxDistance:      %.3f", d.FlybyDamageResponseMaximumDistance);

    // --- Noise ---
    g_pSystem->Debug->Log("  [Noise]");
    g_pSystem->Debug->Log("    ImpactNoise:                 %u", (uint32_t)d.ImpactNoise);
    g_pSystem->Debug->Log("    DetonationNoise:             %u", (uint32_t)d.DetonationNoise);

    // --- Flags ---
    g_pSystem->Debug->Log("  [Flags_2: 0x%08X]", d.Flags_2);
    g_pSystem->Debug->Log("    OrientedAlongVelocity:       %d", (d.Flags_2 >> 0) & 1);
    g_pSystem->Debug->Log("    MustUseBallisticAiming:      %d", (d.Flags_2 >> 1) & 1);
    g_pSystem->Debug->Log("    DetonationMaxTimeIfAttached: %d", (d.Flags_2 >> 2) & 1);
    g_pSystem->Debug->Log("    DamageScalesOnDistance:      %d", (d.Flags_2 >> 3) & 1);
    g_pSystem->Debug->Log("    SteeringAdjustsOrientation:  %d", (d.Flags_2 >> 4) & 1);
    g_pSystem->Debug->Log("    DontNoiseSteering:           %d", (d.Flags_2 >> 5) & 1);
    g_pSystem->Debug->Log("    CanTrackBehindItself:        %d", (d.Flags_2 >> 6) & 1);
    g_pSystem->Debug->Log("    RobotronSteering:            %d", (d.Flags_2 >> 7) & 1);
    g_pSystem->Debug->Log("    AffectedByPhantomVolumes:    %d", (d.Flags_2 >> 8) & 1);
    g_pSystem->Debug->Log("    NotifiesTargetUnits:         %d", (d.Flags_2 >> 10) & 1);
    g_pSystem->Debug->Log("    UseGroundDetonWhenAttached:  %d", (d.Flags_2 >> 11) & 1);
    g_pSystem->Debug->Log("    AIMinorTrackingThreat:       %d", (d.Flags_2 >> 12) & 1);
    g_pSystem->Debug->Log("    DangerousWhenInactive:       %d", (d.Flags_2 >> 13) & 1);
    g_pSystem->Debug->Log("    AiStimulusWhenAttached:      %d", (d.Flags_2 >> 14) & 1);
    g_pSystem->Debug->Log("    OverpenetrationDetonation:   %d", (d.Flags_2 >> 15) & 1);

    // --- OldMaterialResponses ---
    g_pSystem->Debug->Log("  [OldMaterialResponses: %zu]", proj.OldMaterialResponses.size());
    for (size_t i = 0; i < proj.OldMaterialResponses.size(); ++i)
    {
        const auto& r = proj.OldMaterialResponses[i];
        g_pSystem->Debug->Log("    [%zu] DefaultResponse:%u PotentialResponse:%u"
            " ChanceFraction:%.3f ParallelFriction:%.3f PerpendicularFriction:%.3f"
            " InitialFriction:%.3f AngularNoise:%.3f VelocityNoise:%.3f",
            i, (uint32_t)r.DefaultResponse, (uint32_t)r.PotentialResponse,
            r.ChanceFraction, r.ParallelFriction, r.PerpendicularFriction,
            r.InitialFriction, r.AngularNoise, r.VelocityNoise);
    }

    // --- MaterialResponse ---
    g_pSystem->Debug->Log("  [MaterialResponse: %zu]", proj.MaterialResponse.size());
    for (size_t i = 0; i < proj.MaterialResponse.size(); ++i)
    {
        const auto& r = proj.MaterialResponse[i];
        g_pSystem->Debug->Log("    [%zu] Response:%u ChanceFraction:%.3f"
            " ParallelFriction:%.3f PerpendicularFriction:%.3f"
            " InitialFriction:%.3f AngularNoise:%.3f VelocityNoise:%.3f",
            i, (uint32_t)r.Response, r.ChanceFraction,
            r.ParallelFriction, r.PerpendicularFriction,
            r.InitialFriction, r.AngularNoise, r.VelocityNoise);
    }

    // --- BruteGrenade ---
    g_pSystem->Debug->Log("  [BruteGrenade: %zu]", proj.BruteGrenade.size());
    for (size_t i = 0; i < proj.BruteGrenade.size(); ++i)
    {
        const auto& bg = proj.BruteGrenade[i];
        g_pSystem->Debug->Log("    [%zu] MinAngVel:%.3f MaxAngVel:%.3f SpinAngVel:%.3f"
            " AngDamping:%.3f AttachSampleRadius:%.3f AttachAccelK:%.3f",
            i, bg.MinimumAngularVelocity, bg.MaximumAngularVelocity,
            bg.SpinAngularVelocity, bg.AngularDamping,
            bg.AttachSampleRadius, bg.AttachAccelerationK);
    }

    // --- FireBombGrenade ---
    g_pSystem->Debug->Log("  [FireBombGrenade: %zu]", proj.FireBombGrenade.size());
    for (size_t i = 0; i < proj.FireBombGrenade.size(); ++i)
    {
        g_pSystem->Debug->Log("    [%zu] ProjectionOffset:%.3f",
            i, proj.FireBombGrenade[i].ProjectionOffset);
    }
    
    // --- ConicalSpread ---
    g_pSystem->Debug->Log("  [ConicalSpread: %zu]", proj.ConicalSpread.size());
    for (size_t i = 0; i < proj.ConicalSpread.size(); ++i)
    {
        const auto& cs = proj.ConicalSpread[i];
        g_pSystem->Debug->Log("    [%zu] YawCount:%d PitchCount:%d"
            " DistributionExponent:%.3f Spread:%.3f",
            i, cs.YawCount, cs.PitchCount,
            cs.DistributionExponent, cs.Spread);
    }

    g_pSystem->Debug->Log("=== END PROJ: %s ===", tagName.c_str());
}