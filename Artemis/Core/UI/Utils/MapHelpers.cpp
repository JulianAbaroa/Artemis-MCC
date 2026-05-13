#include "pch.h"

// Header.
#include "MapHelpers.h"

#include <algorithm>
#include <cmath>

// --- Coordinate transform ---

ImVec2 UI_MapHelpers::ToCanvas(float wx, float wy, const MapTransform& transform)
{
    float nx = (wx - transform.WorldMinX) *
        transform.Scale + transform.OffsetX;

    float ny = transform.CanvasSize.y - transform.OffsetY
        - (wy - transform.WorldMinY) * transform.Scale;

    return ImVec2(transform.CanvasPos.x + nx,
        transform.CanvasPos.y + ny);
}

// --- Math ---

std::array<float, 3> UI_MapHelpers::Normalize(const std::array<float, 3>& v)
{
    float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len < 0.0001f) return { 0.0f, 0.0f, 1.0f };
    return { v[0] / len, v[1] / len, v[2] / len };
}

std::array<float, 3> UI_MapHelpers::Cross(const std::array<float, 3>& a,
    const std::array<float, 3>& b)
{
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    };
}

// --- Convex hull ---

std::vector<int> UI_MapHelpers::ComputeConvexHull(const std::vector<ImVec2>& pts)
{
    int n = static_cast<int>(pts.size());
    std::vector<int> hull;

    int start = 0;
    for (int j = 1; j < n; ++j)
        if (pts[j].x < pts[start].x ||
            (pts[j].x == pts[start].x && pts[j].y < pts[start].y))
            start = j;

    int current = start;
    do
    {
        hull.push_back(current);
        int next = (current + 1) % n;
        for (int j = 0; j < n; ++j)
        {
            float cross =
                (pts[next].x - pts[current].x) * (pts[j].y - pts[current].y) -
                (pts[next].y - pts[current].y) * (pts[j].x - pts[current].x);
            if (cross < 0.0f) next = j;
        }
        current = next;
    } while (current != start && static_cast<int>(hull.size()) < n);

    return hull;
}

void UI_MapHelpers::DrawConvexHullFilled(ImDrawList* draw,
    const std::vector<ImVec2>& pts, ImU32 color)
{
    int n = static_cast<int>(pts.size());
    if (n < 3) return;

    std::vector<int> hull = ComputeConvexHull(pts);
    int hullSize = static_cast<int>(hull.size());

    for (int j = 1; j < hullSize - 1; ++j)
        draw->AddTriangleFilled(
            pts[hull[0]], pts[hull[j]], pts[hull[j + 1]], color);
}

void UI_MapHelpers::DrawConvexHullDashed(ImDrawList* draw,
    const std::vector<ImVec2>& pts, ImU32 color, float thickness)
{
    std::vector<int> hull = ComputeConvexHull(pts);
    int hullSize = static_cast<int>(hull.size());

    for (int j = 0; j < hullSize; ++j)
    {
        ImVec2 from = pts[hull[j]];
        ImVec2 to = pts[hull[(j + 1) % hullSize]];

        float dx = to.x - from.x;
        float dy = to.y - from.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.001f) continue;

        float dash = 6.0f;
        float gap = 4.0f;
        float step = dash + gap;
        int   segs = static_cast<int>(len / step);

        for (int s = 0; s < segs; ++s)
        {
            float  t0 = (s * step) / len;
            float  t1 = (std::min)((s * step + dash) / len, 1.0f);
            ImVec2 p0 = ImVec2(from.x + dx * t0, from.y + dy * t0);
            ImVec2 p1 = ImVec2(from.x + dx * t1, from.y + dy * t1);
            draw->AddLine(p0, p1, color, thickness);
        }
    }
}

void UI_MapHelpers::DrawPhmoRigidBody(ImDrawList* draw,
    const PhmoRigidBody& rb,
    const std::array<float, 3>& position,
    const std::array<float, 3>& forward,
    const std::array<float, 3>& up,
    const MapTransform& transform,
    ImU32 color, ImU32 fillColor)
{
    // Derive object-space axes from Forward and Up.
    // right = forward x up (left-hand to match Halo's coordinate system).
    const auto fwd = Normalize(forward);
    const auto upN = Normalize(up);
    const auto rgt = Normalize(Cross(fwd, upN));

    // Transforms a local-space XYZ offset to world XY.
    auto toWorld = [&](float lx, float ly, float lz)
        -> std::array<float, 2>
        {
            return {
                position[0] + rgt[0] * lx + fwd[0] * ly + upN[0] * lz,
                position[1] + rgt[1] * lx + fwd[1] * ly + upN[1] * lz,
            };
        };

    // If NodeIndex > 0, we don't have the bone offset — fall back to
    // BoundingSphere only.
    if (rb.NodeIndex > 0)
    {
        auto [wx, wy] = toWorld(
            rb.BoundingSphereOffset.X,
            rb.BoundingSphereOffset.Y,
            rb.BoundingSphereOffset.Z);

        ImVec2 center = ToCanvas(wx, wy, transform);
        float  r = (std::max)(
            rb.BoundingSphereRadius * transform.Scale, 2.0f);

        draw->AddCircleFilled(center, r, fillColor);
        draw->AddCircle(center, r, color, 16, 1.0f);
        return;
    }

    // NodeIndex == -1 or 0 — transform shapes directly.
    for (const PhmoShape& shape : rb.Shapes)
    {
        switch (shape.Type)
        {
        case PhmoShapeType::Sphere:
        {
            auto [wx, wy] = toWorld(
                shape.Sphere.Center.X,
                shape.Sphere.Center.Y,
                shape.Sphere.Center.Z);

            ImVec2 center = ToCanvas(wx, wy, transform);
            float  r = (std::max)(
                shape.Sphere.Radius * transform.Scale, 2.0f);

            draw->AddCircleFilled(center, r, fillColor);
            draw->AddCircle(center, r, color, 16, 1.5f);
            break;
        }

        case PhmoShapeType::Pill:
        {
            auto [bx, by] = toWorld(
                shape.Pill.Bottom.X,
                shape.Pill.Bottom.Y,
                shape.Pill.Bottom.Z);

            auto [tx, ty] = toWorld(
                shape.Pill.Top.X,
                shape.Pill.Top.Y,
                shape.Pill.Top.Z);

            ImVec2 bottom = ToCanvas(bx, by, transform);
            ImVec2 top = ToCanvas(tx, ty, transform);
            float  r = (std::max)(
                shape.Pill.Radius * transform.Scale, 2.0f);

            draw->AddCircleFilled(bottom, r, fillColor);
            draw->AddCircleFilled(top, r, fillColor);
            draw->AddCircle(bottom, r, color, 16, 1.5f);
            draw->AddCircle(top, r, color, 16, 1.5f);

            // Connect the two caps with lines on each side.
            float dx = top.x - bottom.x;
            float dy = top.y - bottom.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 0.001f)
            {
                float px = -(dy / len) * r;
                float py = (dx / len) * r;

                draw->AddLine(
                    ImVec2(bottom.x + px, bottom.y + py),
                    ImVec2(top.x + px, top.y + py),
                    color, 1.5f);
                draw->AddLine(
                    ImVec2(bottom.x - px, bottom.y - py),
                    ImVec2(top.x - px, top.y - py),
                    color, 1.5f);
            }
            break;
        }

        case PhmoShapeType::Box:
        {
            // Box has its own local rotation (RotationI/J/K).
            // Compose with object rotation.
            // RotationI/J/K are the box's local axes in object space.
            const PhmoVec3& ri = shape.Box.RotationI;
            const PhmoVec3& rj = shape.Box.RotationJ;

            const float hx = shape.Box.HalfExtents.X;
            const float hy = shape.Box.HalfExtents.Y;

            // Box center in object space.
            const float cx = shape.Box.Center.X;
            const float cy = shape.Box.Center.Y;
            const float cz = shape.Box.Center.Z;

            // 4 corners in object space, using RotationI and RotationJ
            // as the two planar axes (top-down ignores Z contribution).
            auto corner = [&](float si, float sj) -> ImVec2
                {
                    float lx = cx + ri.X * hx * si + rj.X * hy * sj;
                    float ly = cy + ri.Y * hx * si + rj.Y * hy * sj;
                    float lz = cz + ri.Z * hx * si + rj.Z * hy * sj;

                    auto [wx, wy] = toWorld(lx, ly, lz);
                    return ToCanvas(wx, wy, transform);
                };

            ImVec2 p0 = corner(1.0f, 1.0f);
            ImVec2 p1 = corner(-1.0f, 1.0f);
            ImVec2 p2 = corner(-1.0f, -1.0f);
            ImVec2 p3 = corner(1.0f, -1.0f);

            draw->AddQuadFilled(p0, p1, p2, p3, fillColor);
            draw->AddQuad(p0, p1, p2, p3, color, 1.5f);
            break;
        }

        case PhmoShapeType::Polyhedron:
        {
            // No vertex data — use AABB half extents as a circle fallback.
            auto [wx, wy] = toWorld(
                shape.Polyhedron.AABBCenter.X,
                shape.Polyhedron.AABBCenter.Y,
                shape.Polyhedron.AABBCenter.Z);

            ImVec2 center = ToCanvas(wx, wy, transform);
            float  r = (std::max)({
                shape.Polyhedron.AABBHalfExtents.X,
                shape.Polyhedron.AABBHalfExtents.Y,
                shape.Polyhedron.AABBHalfExtents.Z,
                }) * transform.Scale;
            r = (std::max)(r, 2.0f);

            draw->AddCircleFilled(center, r, fillColor);
            draw->AddCircle(center, r, color, 16, 1.5f);
            break;
        }

        case PhmoShapeType::MultiSphere:
        {
            for (const auto& sphere : shape.MultiSphere.Spheres)
            {
                auto [wx, wy] = toWorld(
                    sphere.Center.X,
                    sphere.Center.Y,
                    sphere.Center.Z);

                ImVec2 center = ToCanvas(wx, wy, transform);
                float  r = (std::max)(
                    sphere.Radius * transform.Scale, 2.0f);

                draw->AddCircleFilled(center, r, fillColor);
                draw->AddCircle(center, r, color, 16, 1.5f);
            }
            break;
        }

        default:
            break;
        }
    }
}