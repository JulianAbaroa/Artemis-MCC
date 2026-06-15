#pragma once

// Connects clusters across BSP boundaries by matching Structure Seam
// centroids from different SbspGeometry instances.
//
// This runs after all SbspGeometry objects have been built.  It mutates
// the Clusters inside each geometry by appending SbspCrossLink entries.

#include "Generated/Sbsp/SbspObject.h"

#include <vector>

struct SbspGeometry;
struct SbspVec3;

class System_Logs;

struct Sys_SbspSeamLinker_Deps
{
    System_Logs& System_Logs;
};

class System_SbspSeamLinker
{
public:
    System_SbspSeamLinker(Sys_SbspSeamLinker_Deps deps) : m_Deps(deps) {}
    ~System_SbspSeamLinker() = default;

    void LinkSeams(
        std::vector<SbspGeometry>& geometries,
        const std::vector<const SbspObject*>& sbspObjects,
        float threshold = 0.05f);

private:
    Sys_SbspSeamLinker_Deps m_Deps;

    // --- Helpers ---
    float DistanceSquared(const SbspVec3& a, const SbspVec3& b);
};