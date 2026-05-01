#pragma once

struct SceneryObstacleData;
struct ScenObject;

class System_ScenObstacleBuilder
{
public:
    SceneryObstacleData BuildData(const ScenObject& scen) const;
    // Returns false if this scen has no navigation relevance (policy == None
    // AND not physically simulating AND not a pathfinding obstacle flag set)
    bool IsNavigationRelevant(const ScenObject& scen) const;
};