#include "pch.h"

// Header.
#include "UI_Environment.h"

// States.
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

// Environment.
#include "Core/States/Domain/Environment/State_Environment.h"

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"

#include <algorithm>
#include <numeric>
#include <limits>
#include <cmath>

// --- Public ---

void UI_Environment::FetchState()
{
    auto& environment = *g_pState->Domain->Environment;
    m_CachedInstances = environment.GetActivePhysicsInstances();
}

void UI_Environment::Cleanup()
{
    m_CachedInstances.clear();
}

void UI_Environment::Draw(const MapTransform& transform,
    const MapSelection& selection) const
{
    ImGui::Text("Physics instances: %d",
        static_cast<int>(m_CachedInstances.size()));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    this->DrawSelectionPanel(selection);
}

void UI_Environment::DrawLayers(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection,
    uint32_t visibleLayers) const
{
    auto isVisible = [&](MapLayer layer) {
        return (visibleLayers & static_cast<uint32_t>(layer)) != 0;
        };

    if (isVisible(MapLayer::PhysicsInstances))
        this->DrawPhysicsInstances(draw, transform, selection);
}

void UI_Environment::DrawSelectionPanel(const MapSelection& selection) const
{
    if (selection.Type != MapSelectionType::PhysicsInstance)
    {
        ImGui::TextDisabled("No selection.");
        return;
    }

    const int32_t i = selection.Index;
    if (i < 0 || i >= static_cast<int32_t>(m_CachedInstances.size())) return;

    ImGui::Indent(5.0f);
    this->DrawSelectionPhysicsInstance(m_CachedInstances[i]);
    ImGui::Unindent(5.0f);
}

void UI_Environment::CollectCandidates(const MapTransform& transform,
    std::vector<MapCandidate>& candidates, uint32_t visibleLayers) const
{
    auto isVisible = [&](MapLayer layer) {
        return (visibleLayers & static_cast<uint32_t>(layer)) != 0;
        };

    if (!isVisible(MapLayer::PhysicsInstances)) return;

    const int32_t prevSize = static_cast<int32_t>(candidates.size());
    const ImVec2 mouse = ImGui::GetIO().MousePos;

    for (int32_t i = 0;
        i < static_cast<int32_t>(m_CachedInstances.size()); ++i)
    {
        const auto& instance = m_CachedInstances[i];

        ImVec2 center = UI_MapHelpers::ToCanvas(
            instance.Position[0], instance.Position[1], transform);

        float worldRadius = (std::max)(
            (instance.CollBoundsMax[0] - instance.CollBoundsMin[0]) * 0.5f,
            (instance.CollBoundsMax[1] - instance.CollBoundsMin[1]) * 0.5f);

        float screenRadius = (std::max)(worldRadius * transform.Scale, 4.0f);

        float dx = mouse.x - center.x;
        float dy = mouse.y - center.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= screenRadius)
            candidates.push_back({ MapSelectionType::PhysicsInstance,
                i, screenRadius });
    }

    g_pSystem->Debug->Log("[UI_Environment] CollectCandidates: instances=%d added=%d",
        (int)m_CachedInstances.size(),
        (int)candidates.size() - prevSize);
}

void UI_Environment::GetWorldBounds(float& outMinX, float& outMinY,
    float& outMaxX, float& outMaxY) const
{
    for (const auto& instance : m_CachedInstances)
    {
        outMinX = (std::min)(outMinX, instance.Position[0]);
        outMinY = (std::min)(outMinY, instance.Position[1]);
        outMaxX = (std::max)(outMaxX, instance.Position[0]);
        outMaxY = (std::max)(outMaxY, instance.Position[1]);
    }
}

// --- Draw layers ---

void UI_Environment::DrawPhysicsInstances(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0;
        i < static_cast<int32_t>(m_CachedInstances.size()); ++i)
    {
        const auto& instance = m_CachedInstances[i];
        bool selected = (selection.Type == MapSelectionType::PhysicsInstance &&
            selection.Index == i);

        ImU32 color = selected ? IM_COL32(100, 220, 255, 255)
            : IM_COL32(60, 180, 220, 180);
        ImU32 fillColor = selected ? IM_COL32(100, 220, 255, 60)
            : IM_COL32(60, 180, 220, 25);

        if (!instance.WorldRigidBodies.empty())
        {
            for (const auto& wrb : instance.WorldRigidBodies)
                DrawWorldRigidBody(draw, transform, wrb, color, fillColor);
        }
        else
        {
            DrawCollFallback(draw, transform, instance, color, fillColor);
        }

        // Center dot always.
        ImVec2 center = UI_MapHelpers::ToCanvas(
            instance.Position[0], instance.Position[1], transform);
        draw->AddCircleFilled(center, 3.0f, color);
    }
}

void UI_Environment::DrawWorldRigidBody(ImDrawList* draw,
    const MapTransform& transform,
    const WorldRigidBody& wrb,
    ImU32 color, ImU32 fillColor)
{
    for (const auto& ws : wrb.Shapes)
        DrawWorldShape(draw, transform, ws, color, fillColor);
}

void UI_Environment::DrawWorldShape(ImDrawList* draw,
    const MapTransform& transform,
    const WorldShape& ws,
    ImU32 color, ImU32 fillColor)
{
    switch (ws.Type)
    {
    case WorldShapeType::Sphere:
    {
        ImVec2 center = UI_MapHelpers::ToCanvas(
            ws.SphereCenter[0], ws.SphereCenter[1], transform);
        float r = ws.SphereRadius * transform.Scale;
        if (r < 1.0f) r = 1.0f;
        draw->AddCircleFilled(center, r, fillColor);
        draw->AddCircle(center, r, color, 32, 1.5f);
        break;
    }
    case WorldShapeType::Pill:
    {
        // In top-down: we draw capsule as two circles + rectangle
        // between Bottom and Top projected in XY.
        ImVec2 b = UI_MapHelpers::ToCanvas(
            ws.PillBottom[0], ws.PillBottom[1], transform);
        ImVec2 t = UI_MapHelpers::ToCanvas(
            ws.PillTop[0], ws.PillTop[1], transform);
        float r = ws.PillRadius * transform.Scale;
        if (r < 1.0f) r = 1.0f;

        // If Bottom and Top are almost equal in XY, draw only a circle.
        float dx = t.x - b.x, dy = t.y - b.y;
        float len = std::sqrt(dx * dx + dy * dy);

        if (len < 1.0f)
        {
            draw->AddCircleFilled(b, r, fillColor);
            draw->AddCircle(b, r, color, 32, 1.5f);
        }
        else
        {
            // Perpendicular to the axis of the capsule in screen-space.
            float nx = -dy / len, ny = dx / len;

            ImVec2 p0 = { b.x + nx * r, b.y + ny * r };
            ImVec2 p1 = { b.x - nx * r, b.y - ny * r };
            ImVec2 p2 = { t.x - nx * r, t.y - ny * r };
            ImVec2 p3 = { t.x + nx * r, t.y + ny * r };

            draw->AddQuadFilled(p0, p1, p2, p3, fillColor);
            draw->AddQuad(p0, p1, p2, p3, color, 1.5f);
            draw->AddCircleFilled(b, r, fillColor);
            draw->AddCircle(b, r, color, 16, 1.5f);
            draw->AddCircleFilled(t, r, fillColor);
            draw->AddCircle(t, r, color, 16, 1.5f);
        }
        break;
    }
    case WorldShapeType::Box:
    {
        // 8 corners world-space -> project XY -> convex hull of the 8 points.
        // In top, down projection, the box can be rotated in Z, we take
        // the 4 unique corners in XY (ignoring Z).
        // We use the 8 corners and create a simple 2D convex hull.

        ImVec2 pts[8];
        for (int i = 0; i < 8; ++i)
            pts[i] = UI_MapHelpers::ToCanvas(
                ws.BoxCorners[i][0], ws.BoxCorners[i][1], transform);

        // Convex 2D hull for 8 points (simple gift wrapping).
        // For 8 points, inline wrapping is inexpensive enough.
        auto cross2d = [](ImVec2 O, ImVec2 A, ImVec2 B) {
            return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
            };

        // Sort by x then y.
        int idx[8] = { 0,1,2,3,4,5,6,7 };
        std::sort(idx, idx + 8, [&](int a, int b) {
            return pts[a].x < pts[b].x ||
                (pts[a].x == pts[b].x && pts[a].y < pts[b].y);
            });

        ImVec2 hull[16];
        int hullSize = 0;

        // Lower hull.
        for (int i = 0; i < 8; ++i)
        {
            while (hullSize >= 2 &&
                cross2d(hull[hullSize - 2], hull[hullSize - 1], pts[idx[i]]) <= 0)
                --hullSize;
            hull[hullSize++] = pts[idx[i]];
        }
        // Upper hull.
        int lower = hullSize + 1;
        for (int i = 6; i >= 0; --i)
        {
            while (hullSize >= lower &&
                cross2d(hull[hullSize - 2], hull[hullSize - 1], pts[idx[i]]) <= 0)
                --hullSize;
            hull[hullSize++] = pts[idx[i]];
        }
        --hullSize; // last period = first

        if (hullSize >= 3)
        {
            draw->AddConvexPolyFilled(hull, hullSize, fillColor);
            draw->AddPolyline(hull, hullSize, color, ImDrawFlags_Closed, 1.5f);
        }
        break;
    }
    case WorldShapeType::Polyhedron:
    {
        if (ws.PolyhedronVertices.size() < 3) break;

        // Project all vertices onto XY and make the hull convex.
        std::vector<ImVec2> pts;
        pts.reserve(ws.PolyhedronVertices.size());
        for (const auto& v : ws.PolyhedronVertices)
            pts.push_back(UI_MapHelpers::ToCanvas(v[0], v[1], transform));

        auto cross2d = [](ImVec2 O, ImVec2 A, ImVec2 B) {
            return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
            };

        int n = static_cast<int>(pts.size());
        std::vector<int> idx(n);
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(), [&](int a, int b) {
            return pts[a].x < pts[b].x ||
                (pts[a].x == pts[b].x && pts[a].y < pts[b].y);
            });

        std::vector<ImVec2> hull;
        hull.reserve(n * 2);

        // Lower.
        for (int i = 0; i < n; ++i)
        {
            while (hull.size() >= 2 &&
                cross2d(hull[hull.size() - 2], hull[hull.size() - 1],
                    pts[idx[i]]) <= 0)
                hull.pop_back();
            hull.push_back(pts[idx[i]]);
        }
        // Upper.
        int lower = static_cast<int>(hull.size()) + 1;
        for (int i = n - 2; i >= 0; --i)
        {
            while (static_cast<int>(hull.size()) >= lower &&
                cross2d(hull[hull.size() - 2], hull[hull.size() - 1],
                    pts[idx[i]]) <= 0)
                hull.pop_back();
            hull.push_back(pts[idx[i]]);
        }
        hull.pop_back();

        if (static_cast<int>(hull.size()) >= 3)
        {
            draw->AddConvexPolyFilled(hull.data(),
                static_cast<int>(hull.size()), fillColor);
            draw->AddPolyline(hull.data(),
                static_cast<int>(hull.size()),
                color, ImDrawFlags_Closed, 1.5f);
        }
        break;
    }
    default:
        break;
    }
}

void UI_Environment::DrawCollFallback(ImDrawList* draw,
    const MapTransform& transform,
    const ActivePhysicsInstance& instance,
    ImU32 color, ImU32 fillColor)
{
    const bool hasBounds =
        (instance.CollBoundsMax[0] - instance.CollBoundsMin[0]) > 0.001f ||
        (instance.CollBoundsMax[1] - instance.CollBoundsMin[1]) > 0.001f;

    if (hasBounds)
    {
        const auto fwd = UI_MapHelpers::Normalize(instance.Forward);
        const float rx = fwd[0], ry = fwd[1];
        const float ux = -ry, uy = rx;

        const float cornersLocal[4][2] = {
            { instance.CollBoundsMin[0], instance.CollBoundsMin[1] },
            { instance.CollBoundsMax[0], instance.CollBoundsMin[1] },
            { instance.CollBoundsMax[0], instance.CollBoundsMax[1] },
            { instance.CollBoundsMin[0], instance.CollBoundsMax[1] },
        };

        ImVec2 pts[4];
        for (int j = 0; j < 4; ++j)
        {
            float wx = instance.Position[0]
                + rx * cornersLocal[j][0] + ux * cornersLocal[j][1];
            float wy = instance.Position[1]
                + ry * cornersLocal[j][0] + uy * cornersLocal[j][1];
            pts[j] = UI_MapHelpers::ToCanvas(wx, wy, transform);
        }

        draw->AddQuadFilled(pts[0], pts[1], pts[2], pts[3], fillColor);
        draw->AddQuad(pts[0], pts[1], pts[2], pts[3], color, 1.5f);
    }
    else
    {
        ImVec2 center = UI_MapHelpers::ToCanvas(
            instance.Position[0], instance.Position[1], transform);
        draw->AddCircleFilled(center, 4.0f, fillColor);
        draw->AddCircle(center, 4.0f, color, 8, 1.5f);
    }
}

// --- Selection panel ---

void UI_Environment::DrawSelectionPhysicsInstance(
    const ActivePhysicsInstance& instance) const
{
    ImGui::TextColored(ImVec4(0.4f, 0.86f, 1.0f, 1.0f), "Physics Instance");
    ImGui::Separator();

    ImGui::Text("%s", instance.TagName.c_str());
    ImGui::Text("Handle:   0x%08X", instance.Handle);

    ImGui::Spacing();
    ImGui::Text("Position: %.2f, %.2f, %.2f",
        instance.Position[0], instance.Position[1], instance.Position[2]);
    ImGui::Text("Forward:  %.2f, %.2f, %.2f",
        instance.Forward[0], instance.Forward[1], instance.Forward[2]);
    ImGui::Text("Up:       %.2f, %.2f, %.2f",
        instance.Up[0], instance.Up[1], instance.Up[2]);

    ImGui::Spacing();
    ImGui::TextDisabled("Coll AABB (local):");
    ImGui::Text("  Min: %.2f, %.2f, %.2f",
        instance.CollBoundsMin[0],
        instance.CollBoundsMin[1],
        instance.CollBoundsMin[2]);
    ImGui::Text("  Max: %.2f, %.2f, %.2f",
        instance.CollBoundsMax[0],
        instance.CollBoundsMax[1],
        instance.CollBoundsMax[2]);

    ImGui::Spacing();
    if (instance.Phmo)
    {
        ImGui::TextDisabled("Phmo:");
        ImGui::Text("  Rigid bodies: %d",
            static_cast<int>(instance.Phmo->RigidBodies.size()));

        for (int32_t i = 0;
            i < static_cast<int32_t>(instance.Phmo->RigidBodies.size()); ++i)
        {
            const auto& rb = instance.Phmo->RigidBodies[i];

            ImGui::Spacing();
            ImGui::Text("  [%d] Node %d | Shapes: %d", i,
                rb.NodeIndex,
                static_cast<int>(rb.Shapes.size()));

            ImGui::Text("       Bounding sphere: r=%.2f",
                rb.BoundingSphereRadius);

            for (const auto& shape : rb.Shapes)
            {
                switch (shape.Type)
                {
                case PhmoShapeType::Sphere:
                    ImGui::Text("       Sphere  r=%.2f",
                        shape.Sphere.Radius);
                    break;
                case PhmoShapeType::Pill:
                    ImGui::Text("       Pill    r=%.2f",
                        shape.Pill.Radius);
                    break;
                case PhmoShapeType::Box:
                    ImGui::Text("       Box     %.2f x %.2f x %.2f",
                        shape.Box.HalfExtents.X * 2.0f,
                        shape.Box.HalfExtents.Y * 2.0f,
                        shape.Box.HalfExtents.Z * 2.0f);
                    break;
                case PhmoShapeType::Polyhedron:
                    ImGui::Text("       Polyhedron");
                    break;
                case PhmoShapeType::MultiSphere:
                    ImGui::Text("       MultiSphere  count=%d",
                        static_cast<int>(shape.MultiSphere.Spheres.size()));
                    break;
                default:
                    ImGui::Text("       Unknown shape");
                    break;
                }
            }
        }
    }
    else
    {
        ImGui::TextDisabled("Phmo: none");
    }

    ImGui::Spacing();
    ImGui::TextDisabled("World rigid bodies:");
    ImGui::Text("  Count: %d", (int)instance.WorldRigidBodies.size());
    for (int32_t j = 0;
        j < static_cast<int32_t>(instance.WorldRigidBodies.size()); ++j)
    {
        const auto& wrb = instance.WorldRigidBodies[j];
        ImGui::Text("  [%d] Shapes: %d | BSphere r=%.2f",
            j,
            (int)wrb.Shapes.size(),
            wrb.BoundingSphereRadius);
        for (int32_t k = 0;
            k < static_cast<int32_t>(wrb.Shapes.size()); ++k)
        {
            const auto& ws = wrb.Shapes[k];
            switch (ws.Type)
            {
            case WorldShapeType::Sphere:
                ImGui::Text("      [%d] Sphere r=%.2f", k, ws.SphereRadius);
                break;
            case WorldShapeType::Pill:
                ImGui::Text("      [%d] Pill r=%.2f", k, ws.PillRadius);
                break;
            case WorldShapeType::Box:
                ImGui::Text("      [%d] Box", k);
                break;
            case WorldShapeType::Polyhedron:
                ImGui::Text("      [%d] Polyhedron verts=%d", k,
                    (int)ws.PolyhedronVertices.size());
                break;
            default:
                ImGui::Text("      [%d] Unknown", k);
                break;
            }
        }
    }
}