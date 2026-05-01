#include "pch.h"
#include "Core/Systems/Domain/Navigation/Mach/System_MachDataBuilder.h"
#include "Core/Types/Domain/Domains/Navigation/MachData.h"
#include "Generated/Mach/MachObject.h"

MachineData System_MachDataBuilder::BuildData(const MachObject& mach) const
{
    MachineData out{};
    const auto& d = mach.Data;

    out.TagName = mach.TagName;

    out.BoundingRadius = d.BoundingRadius;
    out.Type = static_cast<MachineType>(d.Type);
    out.PathfindingPolicy = static_cast<MachPathfindingPolicy>(d.PathfindingPolicy);
    out.CollisionResponse = static_cast<MachCollisionResponse>(d.CollisionResponse);

    out.IsElevator = (d.Flags_3 & (1u << 2)) != 0;
    out.PositionInterpolation = (d.Flags_2 & (1u << 2)) != 0;

    out.AutomaticActivationRadius = d.AutomaticActivationRadius;
    out.DoorOpenTime = d.DoorOpenTime;

    out.PositionTransitionTime = d.PositionTransitionTime;
    out.PositionAccelerationTime = d.PositionAccelerationTime;
    out.PowerAccelerationTime = d.PowerAccelerationTime;

    return out;
}