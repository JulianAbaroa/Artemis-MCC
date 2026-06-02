#pragma once

// Converts a fully-deserialized SbspObject into a SbspGeometry that is
// suitable for AI navigation and reinforcement learning observations.

#include "Generated/Sbsp/SbspObject.h"

struct SbspGeometry;
struct SbspVec3;
struct SbspVec4;
struct RangeF;
struct Vec3;
struct Vec4;

class State_MapZone;
class State_MapPlay;
class State_MapLbsp;
class System_MapReader;
class System_MeshReader;
class System_Logs;

struct SbspBuilder_Dependencies
{
    State_MapZone& State_MapZone;
    State_MapPlay& State_MapPlay;
    State_MapLbsp& State_MapLbsp;
    System_MapReader& System_Header;
    System_MeshReader& System_MeshReader;
    System_Logs& System_Logs;
};

class System_SbspBuilder
{
public:
    System_SbspBuilder(SbspBuilder_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~System_SbspBuilder() = default;

    // Takes a fully-deserialized SbspObject (all std::vector<> fields already 
    // populated by GroupDescriptor) and returns a self-contained SbspGeometry. 
    // The SbspObject can be discarded afterwards if memory is a concern.
    SbspGeometry Build(const SbspObject& sbsp, int32_t sbspIndex);

private:
    SbspBuilder_Dependencies m_Deps;

    void BuildWorldBounds(const SbspObject& sbsp, SbspGeometry& out);
    void BuildPortals(const SbspObject& sbsp, SbspGeometry& out);
    void BuildClusters(const SbspObject& sbsp, SbspGeometry& out);
    void BuildClusterAdjacency(const SbspObject& sbsp, SbspGeometry& out);
    void BuildInstancedGeometry(const SbspObject& sbsp, SbspGeometry& out);
    void BuildCollisionMaterials(const SbspObject& sbsp, SbspGeometry& out);
    void BuildMarkers(const SbspObject& sbsp, SbspGeometry& out);

    // --- Helpers ---

    SbspVec3 MakeVec3(const Vec3& v);
    SbspVec4 MakeVec4(const Vec4& v);
    SbspVec3 RangeFCenter(const RangeF& x, const RangeF& y, const RangeF& z);
};