#include "pch.h"
#include "Core/Systems/Domain/Interactable/Proj/System_ProjDataBuilder.h"
#include "Core/Types/Domain/Domains/Interactable/ProjectileData.h"
#include "Generated/Proj/ProjObject.h"

ProjectileData System_ProjDataBuilder::BuildData(const ProjObject& proj)
{
    ProjectileData out{};
    const auto& d = proj.Data;

    out.TagName = proj.TagName;
    out.Type = this->DeriveType(proj);

    // Kinematics.
    out.InitialVelocity = d.InitialVelocity;
    out.FinalVelocity = d.FinalVelocity;
    out.AirGravityScale = d.AirGravityScale;
    out.WaterGravityScale = d.WaterGravityScale;
    out.AccelerationRangeMin = d.AccelerationRange.Min;
    out.AccelerationRangeMax = d.AccelerationRange.Max;
    out.AirDamageRangeMin = d.AirDamageRange.Min;
    out.AirDamageRangeMax = d.AirDamageRange.Max;
    out.WaterDamageRangeMin = d.WaterDamageRange.Min;
    out.WaterDamageRangeMax = d.WaterDamageRange.Max;

    // Guidance.
    out.GuidedAngularVelocityLower = d.GuidedAngularVelocityLower;
    out.GuidedAngularVelocityUpper = d.GuidedAngularVelocityUpper;
    out.GuidedAngularVelocityAtRest = d.GuidedAngularVelocityAtRest;
    out.TargetedLeadingFraction = d.TargetedLeadingFraction;
    out.OuterRangeErrorRadius = d.GuidedProjectileOuterRangeErrorRadius;
    out.AutoaimLeadingMaxLeadTime = d.AutoaimLeadingMaximumLeadTime;
    out.AiNormalVelocityScale = d.AiNormalVelocityScale;
    out.AiHeroicVelocityScale = d.AiHeroicVelocityScale;
    out.AiLegendaryVelocityScale = d.AiLegendaryVelocityScale;
    out.AiNormalGuidedAngularVelocityScale = d.AiNormalGuidedAngularVelocityScale;

    // Detonation.
    out.CollisionRadius = d.CollisionRadius;
    out.ArmingTime = d.ArmingTime;
    out.TimerMin = d.Timer.Min;
    out.TimerMax = d.Timer.Max;
    out.MinimumVelocity = d.MinimumVelocity;
    out.MaximumRange = d.MaximumRange;
    out.SuperDetonationTime = d.SuperDetonationTime;
    out.BoardingDetonationTime = d.BoardingDetonationTime;
    out.MaxLatchTimeToArm = d.MaximumLatchTimeToArm;
    out.DetonationTimerStarts = d.DetonationTimerStarts;

    // Danger.
    out.DangerRadius = d.DangerRadius;
    out.DangerStimuliRadius = d.DangerStimuliRadius;
    out.FlybyDamageMaxDistance = d.FlybyDamageResponseMaximumDistance;
    out.MaterialEffectRadius = d.MaterialEffectRadius;

    // Noise.
    out.ImpactNoise = d.ImpactNoise;
    out.DetonationNoise = d.DetonationNoise;

    // Flags.
    out.MustUseBallisticAiming = (d.Flags_2 & (1u << 1)) != 0;
    out.IsMinorTrackingThreat = (d.Flags_2 & (1u << 12)) != 0;
    out.DangerousWhenInactive = (d.Flags_2 & (1u << 13)) != 0;

    // Derived.
    out.CanBounce = DeriveCanBounce(proj);

    // Conical spread.
    out.HasConicalSpread = !proj.ConicalSpread.empty();
    if (out.HasConicalSpread)
    {
        const auto& cs = proj.ConicalSpread[0];
        out.ConicalSpreadData.YawCount = cs.YawCount;
        out.ConicalSpreadData.PitchCount = cs.PitchCount;
        out.ConicalSpreadData.DistributionExponent = cs.DistributionExponent;
        out.ConicalSpreadData.Spread = cs.Spread;
    }

    return out;
}

ProjectileType System_ProjDataBuilder::DeriveType(const ProjObject& proj)
{
    if (!proj.BruteGrenade.empty())    return ProjectileType::BruteGrenade;
    if (!proj.FireBombGrenade.empty()) return ProjectileType::FireBomb;
    if (!proj.ConicalSpread.empty())   return ProjectileType::ConicalSpread;
    if (proj.Data.GuidedAngularVelocityLower > 0.0f ||
        proj.Data.GuidedAngularVelocityUpper > 0.0f)
        return ProjectileType::Guided;
    return ProjectileType::Ballistic;
}

bool System_ProjDataBuilder::DeriveCanBounce(const ProjObject& proj)
{
    // Response enum: 4 = Bounce, 5 = Bounce (Dud)
    for (const auto& r : proj.MaterialResponse)
        if (r.Response == 4 || r.Response == 5) return true;
    for (const auto& r : proj.OldMaterialResponses)
        if (r.DefaultResponse == 4 || r.DefaultResponse == 5 ||
            r.PotentialResponse == 4 || r.PotentialResponse == 5) return true;
    return false;
}

