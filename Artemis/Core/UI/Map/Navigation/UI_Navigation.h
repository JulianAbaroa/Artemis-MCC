#pragma once

#include "Core/Types/Navigation/NavigationTypes.h"
#include "Core/Types/Navigation/SbspGeometry.h"
#include "Core/Types/Navigation/ScnrZones.h"

#include "Core/UI/Launcher/UI_Tab.h"

#include "Core/UI/Utils/String/EnumToString.h"
#include "Core/UI/Utils/Color/TeamToColor.h"
#include "Core/UI/Utils/Map/MapHelpers.h"
#include "Core/UI/Utils/Map/MapTypes.h"

#include "External/imgui/imgui.h"

#include <vector>
#include <array>

// Dependencies.
class State_Navigation;
class System_Logs;

struct UI_Navigation_Dependencies
{
    State_Navigation& State_Navigation;
    System_Logs& System_Logs;
};

// World-space AABB of the render mesh. Z drives the height shading and
// the occlusion grid; XY drives the top-down projection.
struct RenderMeshBounds
{
    float MinX = 0.0f, MinY = 0.0f, MinZ = 0.0f;
    float MaxX = 0.0f, MaxY = 0.0f, MaxZ = 0.0f;
};

class UI_Navigation : public UI_Tab
{
public:
    UI_Navigation(UI_Navigation_Dependencies dependencies) :
        m_Deps(dependencies) {
    }
    ~UI_Navigation() = default;

    void Draw(const MapTransform& transform,
        const MapSelection& selection) const;

    void DrawLayers(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection, uint32_t visibleLayers) const;
    void DrawSelectionPanel(const MapSelection& selection) const;

    void FetchState();
    void Cleanup();

    void CollectCandidates(const MapTransform& transform,
        std::vector<MapCandidate>& candidates, uint32_t visibleLayers) const;

    void GetWorldBounds(float& outMinX, float& outMinY,
        float& outMaxX, float& outMaxY) const;

    const std::vector<AINavigationCluster>& GetGraph() const { return m_CachedGraph; }

private:
    UI_Navigation_Dependencies m_Deps;

    std::vector<AINavigationCluster> m_CachedGraph;
    std::vector<ActiveObstacle> m_CachedObstacles;
    std::vector<ActiveSpawn> m_CachedSpawns;
    std::vector<ActiveTeleporter> m_CachedTeleporters;
    std::vector<ActiveLift> m_CachedLifts;
    std::vector<ActiveShield> m_CachedShields;
    std::vector<ActiveObjectiveSpawn> m_CachedObjectiveSpawns;
    std::vector<ActiveObjective> m_CachedObjectives;
    std::vector<ActiveDestructible> m_CachedDestructibles;
    std::vector<SbspTriangle> m_CachedRenderMesh;

    ScnrMapZones m_CachedZones;
    bool m_HasZones = false;

    // --- Render mesh: wireframe, accelerated draw ---
    // The mesh is one flat triangle soup that never changes per map. Two
    // structures, both built once in FetchState:
    //
    // 1. Per-triangle AABB + max Z, precomputed so the draw loop doesn't
    //    redo 6 min/max per tri every frame over millions of tris.
    // 2. A spatial grid: each cell holds the indices of triangles whose
    //    XY falls in it. The draw loop visits only the cells the viewport
    //    overlaps, so zoom-in touches a handful of cells instead of
    //    iterating all 3.6M triangles.
    struct MeshTriCache
    {
        float MinX, MinY, MaxX, MaxY; // XY AABB (for cull + screen size)
        float MaxZ;                   // highest vertex (for height color)
    };

    static constexpr int kSpatialGridRes = 128;

    std::vector<MeshTriCache> m_TriCache;          // parallel to mesh
    std::vector<std::vector<int>> m_SpatialGrid;   // res^2 buckets of indices
    float m_GridMinX = 0.0f, m_GridMinY = 0.0f;
    float m_GridInvCellX = 0.0f, m_GridInvCellY = 0.0f;
    float m_MeshMinZ = 0.0f, m_MeshMaxZ = 0.0f;

    // Dedup for the spatial query: a triangle spanning several cells must
    // be drawn once per frame. Instead of clearing a "seen" array each
    // frame, we stamp with an incrementing frame id and compare.
    mutable std::vector<uint32_t> m_TriSeenStamp;
    mutable uint32_t m_FrameStamp = 0;

    void BuildMeshAcceleration(const RenderMeshBounds& bounds);

    // Mesh signature so FetchState (runs every frame) only rebuilds the
    // cached mesh + height grid when the map actually changed.
    size_t m_BakedMeshSize = 0;
    float m_BakedMeshSignature = 0.0f;

    void BuildHeightGrid(const RenderMeshBounds& bounds);

    // --- Selection panel helpers ---

    void DrawSelectionCluster(const AINavigationCluster& c) const;
    void DrawSelectionObstacle(const ActiveObstacle& obs) const;
    void DrawSelectionSpawn(const ActiveSpawn& spawn) const;
    void DrawSelectionTeleporter(const ActiveTeleporter& tele) const;
    void DrawSelectionLift(const ActiveLift& lift) const;
    void DrawSelectionShield(const ActiveShield& shield) const;
    void DrawSelectionObjectiveSpawn(const ActiveObjectiveSpawn& spawn) const;
    void DrawSelectionObjective(const ActiveObjective& obj) const;
    void DrawSelectionDestructible(const ActiveDestructible& dest) const;
    void DrawTeleporterAllowedObjects(AllowedObjects allowed) const;
    void DrawSelectionTriggerVolume(const ScnrTriggerVolume& vol, bool isKill) const;

    // --- Draw layers ---

    void DrawClusters(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection) const;

    void DrawObstacles(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection) const;

    void DrawSpawns(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection) const;

    void DrawTeleporters(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection) const;

    void DrawLifts(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection) const;

    void DrawShields(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection) const;

    void DrawObjectiveSpawns(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection) const;

    void DrawObjectives(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection) const;

    void DrawDestructibles(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection) const;

    void DrawKillZones(ImDrawList* draw, const
        MapTransform& transform) const;

    void DrawSafeZones(ImDrawList* draw, const
        MapTransform& transform) const;

    void DrawTriggerVolume(ImDrawList* draw, const MapTransform& transform,
        const ScnrTriggerVolume& vol, ImU32 color, ImU32 fillColor) const;

    void DrawRenderMesh(ImDrawList* draw, const MapTransform& transform) const;

    bool PointInTriggerVolumeXY(const ImVec2& mouse,
        const ScnrTriggerVolume& vol, const MapTransform& transform) const;

    float VolumeScreenRadius(const ScnrTriggerVolume& vol,
        const MapTransform& transform) const;

    void DrawZoneShape(ZoneShape zoneShape) const;
};