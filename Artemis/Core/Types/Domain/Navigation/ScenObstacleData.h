#pragma once

#include <string>
#include <cstdint>

// PathfindingPolicy values from ScenData
enum class ScenPathfindingPolicy : uint16_t
{
    CutOut = 0,     // static baked lock on navmesh
    Static = 1,     // static obstacle
    Dynamic = 2,    // dynamic obstacle, the AI ​​must avoid it at runtime
    None = 3,       // no effect on navigation, ignore
};

struct SceneryObstacleData
{
    std::string TagName;

    float BoundingRadius;
    ScenPathfindingPolicy PathfindingPolicy;
    bool PhysicallySimulates;  // can move at runtime
    bool NotAPathfindingObstacle;
};