#include "pch.h"
#include "Core/Systems/Domain/Environment/Bipd/System_BipdDataBuilder.h"
#include "Core/Types/Domain/Domains/Environment/BipdPhysicsData.h"
#include "Generated/Bipd/BipdObject.h"

BipdPhysicsData System_BipdDataBuilder::BuildData(const BipdObject& bipd)
{
    BipdPhysicsData out{};

    out.TagName = bipd.TagName;

    // Physical geometry.
    out.BoundingRadius = bipd.Data.BoundingRadius;
    out.HeightStanding = bipd.Data.HeightStanding;
    out.HeightCrouching = bipd.Data.HeightCrouching;
    out.CollisionRadius = bipd.Data.Radius;

    // Movement.
    out.MaxVelocity = bipd.Data.MaximumVelocity;
    out.MaxSidestepVelocity = bipd.Data.MaximumSidestepVelocity;
    out.Acceleration = bipd.Data.Acceleration;
    out.Deceleration = bipd.Data.Deceleration;
    out.JumpVelocity = bipd.Data.JumpVelocity;
    out.AngularVelocityMax = bipd.Data.AngularVelocityMaximum;

    // Flags, Flags_5: bit 2 = Immune To Falling Damage
    //        Flags_6: bit 4 = Flying
    out.ImmuneToFallDamage = (bipd.Data.Flags_5 & (1u << 2)) != 0;
    out.IsFlying = (bipd.Data.Flags_6 & (1u << 4)) != 0;

    // Combat.
    out.AutoaimWidth = bipd.Data.AutoaimWidth;

    return out;
}