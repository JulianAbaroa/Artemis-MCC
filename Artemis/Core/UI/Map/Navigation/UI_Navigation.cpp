#include "pch.h"

#include "UI_Navigation.h"

#include "Core/States/Domains/Navigation/State_Navigation.h"

#include "Core/Systems/Logs/System_Logs.h"

#include "Core/UI/Utils/Hex/HexFormater.h"

#include <algorithm>
#include <limits>
#include <cmath>

void UI_Navigation::Draw(const MapTransform& transform,
    const MapSelection& selection) const
{
    this->DrawSelectionPanel(selection);
}

void UI_Navigation::DrawSelectionPanel(const MapSelection& selection) const
{
    if (selection.Type == MapSelectionType::None)
    {
        ImGui::TextDisabled("No selection.");
        return;
    }

    const int32_t i = selection.Index;

    ImGui::Indent(5.0f);

    switch (selection.Type)
    {
    case MapSelectionType::Cluster:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedGraph.size()))
        {
            this->DrawSelectionCluster(m_CachedGraph[i]);
        }

        break;
    }

    case MapSelectionType::Obstacle:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedObstacles.size()))
        {
            this->DrawSelectionObstacle(m_CachedObstacles[i]);
        }

        break;
    }

    case MapSelectionType::Spawn:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedSpawns.size()))
        {
            this->DrawSelectionSpawn(m_CachedSpawns[i]);
        }

        break;
    }

    case MapSelectionType::Teleporter:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedTeleporters.size()))
        {
            this->DrawSelectionTeleporter(m_CachedTeleporters[i]);
        }

        break;
    }

    case MapSelectionType::Lift:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedLifts.size()))
        {
            this->DrawSelectionLift(m_CachedLifts[i]);
        }

        break;
    }

    case MapSelectionType::Shield:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedShields.size()))
        {
            this->DrawSelectionShield(m_CachedShields[i]);
        }

        break;
    }

    case MapSelectionType::ObjectiveSpawn:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedObjectiveSpawns.size()))
        {
            this->DrawSelectionObjectiveSpawn(m_CachedObjectiveSpawns[i]);
        }

        break;
    }

    case MapSelectionType::Objective:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedObjectives.size()))
        {
            this->DrawSelectionObjective(m_CachedObjectives[i]);
        }

        break;
    }

    case MapSelectionType::Destructible:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedDestructibles.size()))
        {
            this->DrawSelectionDestructible(m_CachedDestructibles[i]);
        }

        break;
    }

    case MapSelectionType::KillZone:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedZones.KillZones.size()))
        {
            this->DrawSelectionTriggerVolume(m_CachedZones.KillZones[i], true);
        }

        break;
    }

    case MapSelectionType::SafeZone:
    {
        if (i >= 0 && i < static_cast<int32_t>(m_CachedZones.SafeZones.size()))
        {
            this->DrawSelectionTriggerVolume(m_CachedZones.SafeZones[i], false);
        }

        break;
    }

    // TODO: Add mesh/triangle selection.

    default:
        break;
    }

    ImGui::Unindent(5.0f);
}

void UI_Navigation::DrawLayers(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection,
    uint32_t visibleLayers) const
{
    auto isVisible = [&](MapLayer layer) {
        return (visibleLayers & static_cast<uint32_t>(layer)) != 0;
    };

    if (isVisible(MapLayer::Clusters))
    {
        this->DrawClusters(draw, transform, selection);
    }

    if (isVisible(MapLayer::Obstacles))
    {
        this->DrawObstacles(draw, transform, selection);
    }

    if (isVisible(MapLayer::Spawns))
    {
        this->DrawSpawns(draw, transform, selection);
    }

    if (isVisible(MapLayer::Teleporters))
    {
        this->DrawTeleporters(draw, transform, selection);
    }

    if (isVisible(MapLayer::Lifts))
    {
        this->DrawLifts(draw, transform, selection);
    }

    if (isVisible(MapLayer::Shields))
    {
        this->DrawShields(draw, transform, selection);
    }

    if (isVisible(MapLayer::ObjectiveSpawns))
    {
        this->DrawObjectiveSpawns(draw, transform, selection);
    }

    if (isVisible(MapLayer::Objectives))
    {
        this->DrawObjectives(draw, transform, selection);
    }

    if (isVisible(MapLayer::Destructibles))
    {
        this->DrawDestructibles(draw, transform, selection);
    }

    if (isVisible(MapLayer::KillZones))
    {
        this->DrawKillZones(draw, transform);
    }

    if (isVisible(MapLayer::SafeZones))
    {
        this->DrawSafeZones(draw, transform);
    }

    if (isVisible(MapLayer::RenderMesh))
    {
        this->DrawRenderMesh(draw, transform);
    }
}

void UI_Navigation::CollectCandidates(const MapTransform& transform,
    std::vector<MapCandidate>& candidates, uint32_t visibleLayers) const
{
    auto isVisible = [&](MapLayer layer) {
        return (visibleLayers & static_cast<uint32_t>(layer)) != 0;
        };

    const ImVec2 mouse = ImGui::GetIO().MousePos;

    auto tryAdd = [&](MapSelectionType type, int32_t index, float wx,
        float wy, float screenRadius) {
            ImVec2 center = UI_MapHelpers::ToCanvas(wx, wy, transform);

            float dx = mouse.x - center.x;
            float dy = mouse.y - center.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= screenRadius)
            {
                candidates.push_back({ type, index, screenRadius });
            }
        };

    if (isVisible(MapLayer::Clusters))
    {
        const std::vector<AINavigationCluster>& graph = m_CachedGraph;

        for (int32_t i = 0; i < static_cast<int32_t>(graph.size()); ++i)
        {
            const auto& navCluster = graph[i];

            ImVec2 pMin = UI_MapHelpers::ToCanvas(navCluster.BoundsMin[0],
                navCluster.BoundsMin[1], transform);

            ImVec2 pMax = UI_MapHelpers::ToCanvas(navCluster.BoundsMax[0],
                navCluster.BoundsMax[1], transform);

            if (pMin.x > pMax.x) std::swap(pMin.x, pMax.x);
            if (pMin.y > pMax.y) std::swap(pMin.y, pMax.y);

            if (mouse.x >= pMin.x && mouse.x <= pMax.x &&
                mouse.y >= pMin.y && mouse.y <= pMax.y)
            {
                float w = pMax.x - pMin.x;
                float h = pMax.y - pMin.y;

                candidates.push_back({ MapSelectionType::Cluster, i,
                    std::sqrt(w * w + h * h) * 0.5f });
            }
        }
    }

    if (isVisible(MapLayer::Obstacles))
    {
        for (int32_t i = 0; i < static_cast<int32_t>(
            m_CachedObstacles.size()); ++i)
        {
            const auto& obstacle = m_CachedObstacles[i];

            float radius = (std::max)(obstacle.BoundingRadius *
                transform.Scale, 3.0f);

            tryAdd(MapSelectionType::Obstacle, i, obstacle.Position[0],
                obstacle.Position[1], radius);
        }
    }

    if (isVisible(MapLayer::Spawns))
    {
        for (int32_t i = 0; i < static_cast<int32_t>(
            m_CachedSpawns.size()); ++i)
        {
            const auto& spawn = m_CachedSpawns[i];

            tryAdd(MapSelectionType::Spawn, i, spawn.Position[0],
                spawn.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::Teleporters))
    {
        for (int32_t i = 0; i < static_cast<int32_t>(
            m_CachedTeleporters.size()); ++i)
        {
            const auto& teleporter = m_CachedTeleporters[i];

            tryAdd(MapSelectionType::Teleporter, i, teleporter.Position[0],
                teleporter.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::Lifts))
    {
        for (int32_t i = 0; i < static_cast<int32_t>(
            m_CachedLifts.size()); ++i)
        {
            const auto& lift = m_CachedLifts[i];

            tryAdd(MapSelectionType::Lift, i, lift.Position[0],
                lift.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::Shields))
    {
        for (int32_t i = 0; i < static_cast<int32_t>(
            m_CachedShields.size()); ++i)
        {
            const auto& shield = m_CachedShields[i];

            tryAdd(MapSelectionType::Shield, i, shield.Position[0],
                shield.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::ObjectiveSpawns))
    {
        for (int32_t i = 0; i < static_cast<int32_t>(
            m_CachedObjectiveSpawns.size()); ++i)
        {
            const auto& spawn = m_CachedObjectiveSpawns[i];

            tryAdd(MapSelectionType::ObjectiveSpawn, i, spawn.Position[0],
                spawn.Position[1], 7.0f);
        }
    }

    if (isVisible(MapLayer::Objectives))
    {
        for (int32_t i = 0; i < static_cast<int32_t>(
            m_CachedObjectives.size()); ++i)
        {
            const auto& obj = m_CachedObjectives[i];

            tryAdd(MapSelectionType::Objective, i, obj.Position[0],
                obj.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::Destructibles))
    {
        for (int32_t i = 0; i < static_cast<int32_t>(
            m_CachedDestructibles.size()); ++i)
        {
            const auto& dest = m_CachedDestructibles[i];

            tryAdd(MapSelectionType::Destructible, i, dest.Position[0],
                dest.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::KillZones))
    {
        if (m_HasZones)
        {
            for (int32_t i = 0; i < static_cast<int32_t>(
                m_CachedZones.KillZones.size()); ++i)
            {
                const auto& vol = m_CachedZones.KillZones[i];

                if (this->PointInTriggerVolumeXY(mouse, vol, transform))
                {
                    candidates.push_back({
                        MapSelectionType::KillZone, i,
                        this->VolumeScreenRadius(vol, transform)
                        });
                }
            }
        }
    }

    if (isVisible(MapLayer::SafeZones))
    {
        if (m_HasZones)
        {
            for (int32_t i = 0; i < static_cast<int32_t>(
                m_CachedZones.SafeZones.size()); ++i)
            {
                const auto& volume = m_CachedZones.SafeZones[i];

                if (this->PointInTriggerVolumeXY(mouse, volume, transform))
                {
                    candidates.push_back({
                        MapSelectionType::SafeZone, i,
                        this->VolumeScreenRadius(volume, transform)
                        });
                }
            }
        }
    }
}

void UI_Navigation::GetWorldBounds(float& outMinX, float& outMinY,
    float& outMaxX, float& outMaxY) const
{
    const std::vector<AINavigationCluster>& graph = m_CachedGraph;

    for (const auto& navCluster : graph)
    {
        outMinX = (std::min)(outMinX, navCluster.BoundsMin[0]);
        outMinY = (std::min)(outMinY, navCluster.BoundsMin[1]);
        outMaxX = (std::max)(outMaxX, navCluster.BoundsMax[0]);
        outMaxY = (std::max)(outMaxY, navCluster.BoundsMax[1]);
    }
}

void UI_Navigation::FetchState()
{
    m_CachedGraph = m_Deps.State_Navigation.GetNavigationGraph();
    m_CachedObstacles = m_Deps.State_Navigation.GetActiveObstacles();
    m_CachedSpawns = m_Deps.State_Navigation.GetActiveSpawns();
    m_CachedTeleporters = m_Deps.State_Navigation.GetActiveTeleporters();
    m_CachedLifts = m_Deps.State_Navigation.GetActiveLifts();
    m_CachedShields = m_Deps.State_Navigation.GetActiveShields();
    m_CachedObjectiveSpawns = m_Deps.State_Navigation.GetActiveObjectiveSpawns();
    m_CachedObjectives = m_Deps.State_Navigation.GetActiveObjectives();
    m_CachedDestructibles = m_Deps.State_Navigation.GetActiveDestructibles();

    // The render mesh is immutable per map. FetchState runs every frame,
    // so we must NOT copy millions of tris each time. Compute a cheap
    // signature straight from the source; only when it changes do we copy
    // the mesh and rebuild the occlusion height grid.
    {
        const auto& geometries = m_Deps.State_Navigation.GetAllSbspGeometry();

        size_t meshSize = 0;
        float signature = 0.0f;
        const SbspTriangle* firstTri = nullptr;
        const SbspTriangle* lastTri = nullptr;

        for (const auto& geometry : geometries)
        {
            if (!geometry.RenderMesh.empty())
            {
                if (!firstTri) firstTri = &geometry.RenderMesh.front();
                lastTri = &geometry.RenderMesh.back();
            }
            meshSize += geometry.RenderMesh.size();
        }

        if (firstTri && lastTri)
        {
            signature = firstTri->V0.X + firstTri->V0.Y + firstTri->V0.Z
                + lastTri->V2.X + lastTri->V2.Y + lastTri->V2.Z;
        }

        const bool meshChanged = (meshSize != m_BakedMeshSize)
            || (signature != m_BakedMeshSignature);

        if (meshChanged)
        {
            m_CachedRenderMesh.clear();
            m_CachedRenderMesh.reserve(meshSize);

            RenderMeshBounds bounds;
            bounds.MinX = bounds.MinY = bounds.MinZ =
                (std::numeric_limits<float>::max)();
            bounds.MaxX = bounds.MaxY = bounds.MaxZ =
                std::numeric_limits<float>::lowest();

            auto expand = [&](const SbspVec3& v) {
                bounds.MinX = (std::min)(bounds.MinX, v.X);
                bounds.MinY = (std::min)(bounds.MinY, v.Y);
                bounds.MinZ = (std::min)(bounds.MinZ, v.Z);
                bounds.MaxX = (std::max)(bounds.MaxX, v.X);
                bounds.MaxY = (std::max)(bounds.MaxY, v.Y);
                bounds.MaxZ = (std::max)(bounds.MaxZ, v.Z);
                };

            for (const auto& geometry : geometries)
            {
                for (const auto& tri : geometry.RenderMesh)
                {
                    m_CachedRenderMesh.push_back(tri);
                    expand(tri.V0);
                    expand(tri.V1);
                    expand(tri.V2);
                }
            }

            this->BuildMeshAcceleration(bounds);

            m_BakedMeshSize = meshSize;
            m_BakedMeshSignature = signature;
        }
    }

    const ScnrMapZones* zones = m_Deps.State_Navigation.GetMapZones();
    if (zones)
    {
        m_CachedZones = *zones;
        m_HasZones = true;
    }
    else
    {
        m_HasZones = false;
    }
}

void UI_Navigation::Cleanup()
{
    m_CachedGraph.clear();
    m_CachedObstacles.clear();
    m_CachedSpawns.clear();
    m_CachedTeleporters.clear();
    m_CachedLifts.clear();
    m_CachedShields.clear();
    m_CachedObjectiveSpawns.clear();
    m_CachedObjectives.clear();
    m_CachedDestructibles.clear();
    m_CachedRenderMesh.clear();

    m_TriCache.clear();
    m_SpatialGrid.clear();

    m_BakedMeshSize = 0;
    m_BakedMeshSignature = 0.0f;

    m_CachedZones = {};
    m_HasZones = false;
}

void UI_Navigation::BuildMeshAcceleration(const RenderMeshBounds& bounds)
{
    const int res = kSpatialGridRes;
    const size_t triCount = m_CachedRenderMesh.size();

    m_MeshMinZ = bounds.MinZ;
    m_MeshMaxZ = bounds.MaxZ;
    m_GridMinX = bounds.MinX;
    m_GridMinY = bounds.MinY;

    const float spanX = (std::max)(bounds.MaxX - bounds.MinX, 1e-4f);
    const float spanY = (std::max)(bounds.MaxY - bounds.MinY, 1e-4f);
    m_GridInvCellX = res / spanX;
    m_GridInvCellY = res / spanY;

    // 1) Per-triangle AABB + max Z, computed once.
    m_TriCache.resize(triCount);

    // Dedup stamps, one per triangle. Reset frame counter on rebuild.
    m_TriSeenStamp.assign(triCount, 0);
    m_FrameStamp = 0;

    // 2) Spatial buckets. A triangle is registered in every cell its XY
    //    AABB overlaps, so a viewport query never misses a triangle.
    m_SpatialGrid.assign(static_cast<size_t>(res) * res, {});

    auto clampCellX = [&](float x) -> int {
        int c = static_cast<int>((x - bounds.MinX) * m_GridInvCellX);
        return (std::min)((std::max)(c, 0), res - 1);
        };
    auto clampCellY = [&](float y) -> int {
        int c = static_cast<int>((y - bounds.MinY) * m_GridInvCellY);
        return (std::min)((std::max)(c, 0), res - 1);
        };

    for (size_t i = 0; i < triCount; ++i)
    {
        const SbspTriangle& tri = m_CachedRenderMesh[i];

        MeshTriCache c;
        c.MinX = (std::min)({ tri.V0.X, tri.V1.X, tri.V2.X });
        c.MaxX = (std::max)({ tri.V0.X, tri.V1.X, tri.V2.X });
        c.MinY = (std::min)({ tri.V0.Y, tri.V1.Y, tri.V2.Y });
        c.MaxY = (std::max)({ tri.V0.Y, tri.V1.Y, tri.V2.Y });
        c.MaxZ = (std::max)({ tri.V0.Z, tri.V1.Z, tri.V2.Z });
        m_TriCache[i] = c;

        const int cx0 = clampCellX(c.MinX);
        const int cx1 = clampCellX(c.MaxX);
        const int cy0 = clampCellY(c.MinY);
        const int cy1 = clampCellY(c.MaxY);

        for (int cy = cy0; cy <= cy1; ++cy)
            for (int cx = cx0; cx <= cx1; ++cx)
                m_SpatialGrid[cy * res + cx].push_back(static_cast<int>(i));
    }
}