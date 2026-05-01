#include "pch.h"
#include "Core/Systems/Domain/Environment/Bipd/System_BipdDataBuilder.h"
#include "Core/Types/Domain/Domains/Environment/BipdPhysicsData.h"
#include "Generated/Bipd/BipdObject.h"

BipdPhysicsData System_BipdDataBuilder::BuildData(const BipdObject& bipd)
{
    BipdPhysicsData out{};
    const auto& d = bipd.Data;

    out.TagName = bipd.TagName;

    // Physics shape
    out.BoundingRadius = d.BoundingRadius;
    out.HeightStanding = d.HeightStanding;
    out.HeightCrouching = d.HeightCrouching;
    out.CollisionRadius = d.Radius;
    out.Mass = d.Mass;

    // Movement
    out.MaxVelocity = d.MaximumVelocity;
    out.MaxSidestepVelocity = d.MaximumSidestepVelocity;
    out.Acceleration = d.Acceleration;
    out.Deceleration = d.Deceleration;
    out.JumpVelocity = d.JumpVelocity;
    out.AngularVelocityMax = d.AngularVelocityMaximum;
    out.AngularAccelerationMax = d.AngularAccelerationMaximum;
    out.CrouchVelocityModifier = d.CrouchVelocityModifier;
    out.StationaryTurningThreshold = d.StationaryTurningThreshold;
    out.MovingTurningSpeed = d.MovingTurningSpeed;

    // Slopes
    out.MaximumSlopeAngle = d.MaximumSlopeAngle;
    out.DownhillFalloffAngle = d.DownhillFalloffAngle;
    out.DownhillCutoffAngle = d.DownhillCutoffAngle;
    out.DownhillVelocityScale = d.DownhillVelocityScale;
    out.UphillFalloffAngle = d.UphillFalloffAngle;
    out.UphillCutoffAngle = d.UphillCutoffAngle;
    out.UphillVelocityScale = d.UphillVelocityScale;
    out.ClimbInflectionAngle = d.ClimbInflectionAngle;

    // Landing
    out.MaximumSoftLandingTime = d.MaximumSoftLandingTime;
    out.MaximumHardLandingTime = d.MaximumHardLandingTime;
    out.MinimumSoftLandingVelocity = d.MinimumSoftLandingVelocity;
    out.MinimumHardLandingVelocity = d.MinimumHardLandingVelocity;
    out.MaximumHardLandingVelocity = d.MaximumHardLandingVelocity;

    // Stun
    out.StunDuration = d.StunDuration;
    out.StunMovementPenalty = d.StunMovementPenalty;
    out.StunTurningPenalty = d.StunTurningPenalty;
    out.StunJumpingPenalty = d.StunJumpingPenalty;
    out.MaximumStunTime = d.MaximumStunTime;

    // Camera heights
    out.StandingCameraHeight = d.StandingCameraHeight;
    out.CrouchingCameraHeight = d.CrouchingCameraHeight;
    out.CrouchTransitionTime = d.CrouchTransitionTime;

    // Combat
    out.HardPingThreshold = d.HardPingThreshold;
    out.HardDeathThreshold = d.HardDeathThreshold;

    // Flags from Flags_6
    out.IsFlying = (d.Flags_6 & (1u << 4)) != 0;
    out.UsePlayerPhysics = (d.Flags_6 & (1u << 2)) != 0;
    out.IsSpherical = (d.Flags_6 & (1u << 1)) != 0;
    out.IsCenteredAtOrigin = (d.Flags_6 & (1u << 0)) != 0;

    return out;
}