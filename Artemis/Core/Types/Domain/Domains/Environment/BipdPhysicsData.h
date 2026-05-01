#pragma once

#include <string>

struct BipdPhysicsData
{
    std::string TagName;

    // Physics shape
    float BoundingRadius;
    float HeightStanding;
    float HeightCrouching;
    float CollisionRadius; // Data.Radius
    float Mass;

    // Movement
    float MaxVelocity;
    float MaxSidestepVelocity;
    float Acceleration;
    float Deceleration;
    float JumpVelocity;
    float AngularVelocityMax;
    float AngularAccelerationMax;
    float CrouchVelocityModifier;
    float StationaryTurningThreshold;
    float MovingTurningSpeed;

    // Slopes
    float MaximumSlopeAngle;
    float DownhillFalloffAngle;
    float DownhillCutoffAngle;
    float DownhillVelocityScale;
    float UphillFalloffAngle;
    float UphillCutoffAngle;
    float UphillVelocityScale;
    float ClimbInflectionAngle;

    // Landing
    float MaximumSoftLandingTime;
    float MaximumHardLandingTime;
    float MinimumSoftLandingVelocity;
    float MinimumHardLandingVelocity;
    float MaximumHardLandingVelocity;

    // Stun
    float StunDuration;
    float StunMovementPenalty;
    float StunTurningPenalty;
    float StunJumpingPenalty;
    float MaximumStunTime;

    // Camera heights
    float StandingCameraHeight;
    float CrouchingCameraHeight;
    float CrouchTransitionTime;

    // Combat
    float HardPingThreshold;
    float HardDeathThreshold;

    // Flags
    bool IsFlying;
    bool UsePlayerPhysics;
    bool IsSpherical;
    bool IsCenteredAtOrigin;
};