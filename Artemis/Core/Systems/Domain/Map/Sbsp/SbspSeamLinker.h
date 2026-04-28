#pragma once

// Connects clusters across BSP boundaries by matching Structure Seam
// centroids from different SbspGeometry instances.
//
// This runs after all SbspGeometry objects have been built.  It mutates
// the Clusters inside each geometry by appending SbspCrossLink entries.

#include "Core/Common/Types/Domain/Map/SbspGeometry.h"
#include "Generated/Sbsp/SbspObject.h"
#include <vector>

class SbspSeamLinker
{
public:
    // Links all BSPs in `geometries` by matching their Structure Seam
    // centroids.  `sbspObjects` must be parallel to `geometries`
    // (i.e. sbspObjects[i] is the source for geometries[i]).
    //
    // threshold — max world-space distance between two centroids for them
    //             to be considered the same seam point. 0.05f is a safe
    //             default for Reach MCC world units.
    void LinkSeams(
        std::vector<SbspGeometry>& geometries,
        const std::vector<const SbspObject*>& sbspObjects,
        float threshold = 0.05f);

private:
    float DistanceSquared(const SbspVec3& a, const SbspVec3& b);
};