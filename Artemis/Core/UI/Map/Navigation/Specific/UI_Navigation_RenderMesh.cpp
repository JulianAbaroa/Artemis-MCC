#include "pch.h"

#include "../UI_Navigation.h"

#include <algorithm>
#include <cmath>
#include <vector>

void UI_Navigation::DrawRenderMesh(ImDrawList* draw,
    const MapTransform& transform) const
{
    if (m_CachedRenderMesh.empty()) return;

    const int res = kSpatialGridRes;
    if (static_cast<int>(m_SpatialGrid.size()) != res * res) return;

    const float spanZ = (std::max)(m_MeshMaxZ - m_MeshMinZ, 1e-4f);
    constexpr float kMinScreenPx = 1.5f;
    constexpr int kBaseAlpha = 110;
    constexpr int kMinAlpha = 18;

    // Visible world-space AABB (frustum).
    auto canvasToWorld = [&](float sx, float sy) -> ImVec2 {
        float wx = (sx - transform.CanvasPos.x - transform.OffsetX)
            / transform.Scale + transform.WorldMinX;

        float wy = transform.WorldMinY - (sy - transform.CanvasPos.y - 
            transform.CanvasSize.y + transform.OffsetY) / transform.Scale;

        return ImVec2(wx, wy);
    };

    const ImVec2 wCorner0 = canvasToWorld(transform.CanvasPos.x, 
        transform.CanvasPos.y);

    const ImVec2 wCorner1 = canvasToWorld(
        transform.CanvasPos.x + transform.CanvasSize.x,
        transform.CanvasPos.y + transform.CanvasSize.y);

    const float viewMinX = (std::min)(wCorner0.x, wCorner1.x);
    const float viewMaxX = (std::max)(wCorner0.x, wCorner1.x);
    const float viewMinY = (std::min)(wCorner0.y, wCorner1.y);
    const float viewMaxY = (std::max)(wCorner0.y, wCorner1.y);

    auto clampCellX = [&](float x) -> int {
        int c = static_cast<int>((x - m_GridMinX) * m_GridInvCellX);
        return (std::min)((std::max)(c, 0), res - 1);
    };

    auto clampCellY = [&](float y) -> int {
        int c = static_cast<int>((y - m_GridMinY) * m_GridInvCellY);
        return (std::min)((std::max)(c, 0), res - 1);
    };

    const int cx0 = clampCellX(viewMinX);
    const int cx1 = clampCellX(viewMaxX);
    const int cy0 = clampCellY(viewMinY);
    const int cy1 = clampCellY(viewMaxY);

    // New frame stamp for dedup (triangles spanning multiple cells).
    ++m_FrameStamp;

    // Screen-space density grid for alpha normalization.
    constexpr int kDensityRes = 128;
    static std::vector<uint16_t> densityGrid;
    densityGrid.assign(kDensityRes * kDensityRes, 0);

    const float canvasW = (std::max)(transform.CanvasSize.x, 1.0f);
    const float canvasH = (std::max)(transform.CanvasSize.y, 1.0f);
    const float densInvX = kDensityRes / canvasW;
    const float densInvY = kDensityRes / canvasH;

    auto densityCell = [&](const ImVec2& pt) -> int {
        int dx = static_cast<int>((pt.x - transform.CanvasPos.x) * densInvX);
        int dy = static_cast<int>((pt.y - transform.CanvasPos.y) * densInvY);

        dx = (std::min)((std::max)(dx, 0), kDensityRes - 1);
        dy = (std::min)((std::max)(dy, 0), kDensityRes - 1);
        return dy * kDensityRes + dx;
    };

    struct DrawTri 
    {
        ImVec2 p0{};
        ImVec2 p1{};
        ImVec2 p2{}; 
        float z{};
        int densCell{};
    };

    static std::vector<DrawTri> visible{};

    visible.clear();
    int32_t lod = 0;

    // Pass 1: walk only the viewport's cells, filter, project.
    for (int cy = cy0; cy <= cy1; ++cy)
    {
        for (int cx = cx0; cx <= cx1; ++cx)
        {
            const std::vector<int>& bucket = m_SpatialGrid[cy * res + cx];

            for (int idx : bucket)
            {
                // Skip if already handled this frame (multi-cell triangle).
                if (m_TriSeenStamp[idx] == m_FrameStamp) continue;
                m_TriSeenStamp[idx] = m_FrameStamp;

                const MeshTriCache& c = m_TriCache[idx];

                // Frustum cull using the precomputed AABB.
                if (c.MaxX < viewMinX || c.MinX > viewMaxX ||
                    c.MaxY < viewMinY || c.MinY > viewMaxY) continue;

                // LOD: drop sub-pixel triangles.
                const float screenW = (c.MaxX - c.MinX) * transform.Scale;
                const float screenH = (c.MaxY - c.MinY) * transform.Scale;
                if ((std::max)(screenW, screenH) < kMinScreenPx)
                {
                    ++lod;
                    continue;
                }

                const SbspTriangle& tri = m_CachedRenderMesh[idx];

                DrawTri d;
                d.p0 = UI_MapHelpers::ToCanvas(tri.V0.X, tri.V0.Y, transform);
                d.p1 = UI_MapHelpers::ToCanvas(tri.V1.X, tri.V1.Y, transform);
                d.p2 = UI_MapHelpers::ToCanvas(tri.V2.X, tri.V2.Y, transform);
                d.z = c.MaxZ;

                ImVec2 centroid((d.p0.x + d.p1.x + d.p2.x) / 3.0f,
                    (d.p0.y + d.p1.y + d.p2.y) / 3.0f);

                d.densCell = densityCell(centroid);

                if (densityGrid[d.densCell] < 0xFFFF)
                {
                    ++densityGrid[d.densCell];
                }

                visible.push_back(d);
            }
        }
    }

    auto heightColor = [&](float z, int alpha) -> ImU32 {
        float t = (z - m_MeshMinZ) / spanZ;
        t = (std::min)((std::max)(t, 0.0f), 1.0f);

        float shade = 0.30f + 0.70f * t;

        int r = static_cast<int>(115 * shade);
        int g = static_cast<int>(200 * shade);
        int b = static_cast<int>(245 * shade);
        return IM_COL32(r, g, b, alpha);
    };

    // Pass 2: draw with density-normalized alpha.
    ImVec2 poly[4];
    for (const auto& d : visible)
    {
        const int count = densityGrid[d.densCell];
        int alpha = kBaseAlpha;
        if (count > 1)
        {
            alpha = static_cast<int>(kBaseAlpha / std::sqrt((float)count));
            alpha = (std::max)(alpha, kMinAlpha);
        }

        poly[0] = d.p0;
        poly[1] = d.p1;
        poly[2] = d.p2;
        poly[3] = d.p0;

        draw->AddPolyline(poly, 4, heightColor(d.z, alpha),
            ImDrawFlags_None, 0.5f);
    }
}