#pragma once

#include <string>
#include <cstdint>

enum class BlocAiSize : uint16_t
{
    Default = 0,
    Tiny = 1,
    Small = 2,
    Medium = 3,
    Large = 4,
    Huge = 5,
    Immobile = 6,
};

struct CrateObstacleData
{
    std::string TagName;

    float BoundingRadius;
    bool NotAPathfindingObstacle;

    // AI cover properties, only valid if HasAiProperties == true
    bool HasAiProperties;
    bool IsDestroyableCover;
    bool IsDynamicCover;
    bool IsDynamicCoverFromCentre;
    bool HasCornerMarkers;
    BlocAiSize AiSize;
};