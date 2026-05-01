#include "pch.h"
#include "Core/Systems/Domain/Navigation/Scen/System_ScenObstacleBuilder.h"
#include "Core/Types/Domain/Domains/Navigation/ScenObstacleData.h"
#include "Generated/Scen/ScenObject.h"

SceneryObstacleData System_ScenObstacleBuilder::BuildData(const ScenObject& scen) const
{
    SceneryObstacleData out{};
    const auto& d = scen.Data;

    out.TagName = scen.TagName;
    out.BoundingRadius = d.BoundingRadius;
    out.PathfindingPolicy = static_cast<ScenPathfindingPolicy>(d.PathfindingPolicy);
    out.PhysicallySimulates = (d.Flags_2 & (1u << 0)) != 0;
    out.NotAPathfindingObstacle = (d.Flags & (1u << 3)) != 0;

    return out;
}

bool System_ScenObstacleBuilder::IsNavigationRelevant(const ScenObject& scen) const
{
    const auto& d = scen.Data;
    const auto policy = static_cast<ScenPathfindingPolicy>(d.PathfindingPolicy);
    const bool physSim = (d.Flags_2 & (1u << 0)) != 0;

    // None policy + no physics = pure decoration, skip
    if (policy == ScenPathfindingPolicy::None && !physSim)
        return false;

    return true;
}