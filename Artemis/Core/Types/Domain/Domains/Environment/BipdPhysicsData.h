#pragma once

#include <string>

struct BipdPhysicsData
{
    std::string TagName;

    // Physical geometry.
    float BoundingRadius;
    float HeightStanding;
    float HeightCrouching;
    float CollisionRadius;

    // Movement.
    float MaxVelocity;
    float MaxSidestepVelocity;
    float Acceleration;
    float Deceleration;
    float JumpVelocity;
    float AngularVelocityMax;

    // Useful flags.
    bool IsFlying;
    bool ImmuneToFallDamage;

    // Combat.
    float AutoaimWidth;
};