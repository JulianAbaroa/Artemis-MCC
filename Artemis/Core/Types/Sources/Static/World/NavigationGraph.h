#pragma once

#include <cstdint>
#include <vector>
#include <array>

struct NavigationLink
{
    // Index in the SbspGeometry this link points to.
    int32_t ClusterIndex;

    // Which SbspGeometry. (-1 = same BSP)
    int32_t SbspIndex;

    // World-space position of the portal/seam.
    std::array<float, 3> Centroid;

    // BoundingRadius of the portal. (how wide the opening is)
    float PassRadius;
};

struct NavigationGraph
{
    int32_t ClusterIndex;
    int32_t SbspIndex;

    bool IsKillZone = false;
    bool IsSafeZone = false;

    std::array<float, 3> Center;
    std::array<float, 3> BoundsMin;
    std::array<float, 3> BoundsMax;

    // All reachable neighbors. (intra + cross-BSP)
    std::vector<NavigationLink> Links;
};

struct NavigationState
{
    // Where the AI is.
    int32_t SelfClusterIndex = -1;
    int32_t SelfSbspIndex = -1;

    // The full cluster graph.
    std::vector<NavigationGraph> Clusters;
};