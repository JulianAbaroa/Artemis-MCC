#pragma once

struct CrateObstacleData;
struct BlocObject;

class System_BlocObstacleBuilder
{
public:
    CrateObstacleData BuildData(const BlocObject& bloc) const;
    bool IsObstacleRelevant(const BlocObject& bloc) const;
};