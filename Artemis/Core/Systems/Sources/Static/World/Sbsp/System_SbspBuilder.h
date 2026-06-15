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

template <typename TObject> class State_MapZone; struct ZoneObject;
template <typename TObject> class State_MapPlay; struct PlayObject;
template <typename TObject> class State_MapLbsp; struct LbspObject;
class System_MapReader;
class System_GeometryReader;
class System_Logs;

struct Sys_SbspBuilder_Deps
{
    State_MapZone<ZoneObject>& State_MapZone;
    State_MapPlay<PlayObject>& State_MapPlay;
    State_MapLbsp<LbspObject>& State_MapLbsp;
    System_MapReader& System_Header;
    System_GeometryReader& System_GeometryReader;
    System_Logs& System_Logs;
};

class System_SbspBuilder
{
public:
    System_SbspBuilder(Sys_SbspBuilder_Deps dependencies) :
        m_Deps(dependencies) {}
    ~System_SbspBuilder() = default;

    SbspGeometry Build(const SbspObject& sbsp, int32_t sbspIndex);

private:
    Sys_SbspBuilder_Deps m_Deps;

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