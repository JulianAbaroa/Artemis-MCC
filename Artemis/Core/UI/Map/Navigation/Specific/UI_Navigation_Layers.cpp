#include "pch.h"

#include "../UI_Navigation.h"

#include <algorithm>
#include <cmath>

void UI_Navigation::DrawClusters(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
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

        bool isSelected = (selection.Type == MapSelectionType::Cluster &&
            selection.Index == i);

        ImU32 fillColor = isSelected ?
            IM_COL32(80, 200, 80, 100) : IM_COL32(40, 80, 120, 50);

        ImU32 borderColor = isSelected ?
            IM_COL32(140, 255, 140, 255) : IM_COL32(80, 160, 220, 180);

        draw->AddRectFilled(pMin, pMax, fillColor);

        draw->AddRect(pMin, pMax, borderColor, 0.0f, 0,
            isSelected ? 2.5f : 1.0f);

        float screenW = pMax.x - pMin.x;
        float screenH = pMax.y - pMin.y;

        if (screenW > 16.0f && screenH > 10.0f)
        {
            ImVec2 center = UI_MapHelpers::ToCanvas(navCluster.Center[0],
                navCluster.Center[1], transform);

            char label[16];
            snprintf(label, sizeof(label), "%d", navCluster.ClusterIndex);

            draw->AddText(ImVec2(center.x - 5.0f, center.y - 7.0f),
                isSelected ? IM_COL32(140, 255, 140, 255)
                : IM_COL32(180, 180, 180, 200), label);
        }
    }

    for (const auto& navCluster : graph)
    {
        for (const auto& link : navCluster.Links)
        {
            if (link.SbspIndex != navCluster.SbspIndex) continue;

            ImVec2 from = UI_MapHelpers::ToCanvas(link.Centroid[0],
                link.Centroid[1], transform);

            ImVec2 to = UI_MapHelpers::ToCanvas(navCluster.Center[0],
                navCluster.Center[1], transform);

            draw->AddLine(from, to, IM_COL32(100, 100, 50, 100), 1.0f);
        }
    }
}

void UI_Navigation::DrawObstacles(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(
        m_CachedObstacles.size()); ++i)
    {
        const auto& obstacle = m_CachedObstacles[i];

        ImVec2 center = UI_MapHelpers::ToCanvas(obstacle.Position[0],
            obstacle.Position[1], transform);

        float radius = (std::max)(obstacle.BoundingRadius *
            transform.Scale, 3.0f);

        bool selected = (selection.Type == MapSelectionType::Obstacle &&
            selection.Index == i);

        draw->AddCircle(center, radius, selected ?
            IM_COL32(255, 200, 80, 255) : IM_COL32(255, 140, 0, 180),
            12, selected ? 2.5f : 1.5f);

        draw->AddCircleFilled(center, radius, selected ?
            IM_COL32(255, 200, 80, 60) : IM_COL32(255, 140, 0, 30));
    }
}

void UI_Navigation::DrawSpawns(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(
        m_CachedSpawns.size()); ++i)
    {
        const auto& spawn = m_CachedSpawns[i];

        ImVec2 center = UI_MapHelpers::ToCanvas(spawn.Position[0],
            spawn.Position[1], transform);

        float radius = 6.0f;
        bool selected = (selection.Type == MapSelectionType::Spawn &&
            selection.Index == i);

        ImU32 color = TeamToColor::TeamColorU32(spawn.Team,
            selected ? 255 : 220);

        ImU32 fillColor = TeamToColor::TeamColorU32(spawn.Team,
            selected ? 120 : 60);

        draw->AddCircleFilled(center, radius, fillColor);
        draw->AddCircle(center, radius, color, 8, selected ? 2.5f : 1.5f);

        ImVec2 tip = ImVec2(center.x + spawn.Forward[0] * radius * 2.0f,
            center.y - spawn.Forward[1] * radius * 2.0f);

        draw->AddLine(center, tip, color, 1.5f);
    }
}

void UI_Navigation::DrawTeleporters(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (const auto& a : m_CachedTeleporters)
    {
        for (const auto& b : m_CachedTeleporters)
        {
            if (a.Handle >= b.Handle) continue;
            if (a.Channel != b.Channel) continue;

            bool aCanSend = (a.Type == TeleporterType::Sender ||
                a.Type == TeleporterType::TwoWay);

            bool bCanSend = (b.Type == TeleporterType::Sender ||
                b.Type == TeleporterType::TwoWay);

            if (!aCanSend && !bCanSend) continue;

            ImVec2 from = UI_MapHelpers::ToCanvas(a.Position[0],
                a.Position[1], transform);

            ImVec2 to = UI_MapHelpers::ToCanvas(b.Position[0],
                b.Position[1], transform);

            float dx = to.x - from.x;
            float dy = to.y - from.y;

            float len = std::sqrt(dx * dx + dy * dy);
            if (len < 0.001f) continue;

            float ndx = dx / len;
            float ndy = dy / len;

            float dash = 6.0f, gap = 4.0f, step = dash + gap;
            int segs = static_cast<int>(len / step);

            for (int s = 0; s < segs; ++s)
            {
                float  t0 = (s * step) / len;
                float  t1 = (std::min)((s * step + dash) / len, 1.0f);

                ImVec2 p0 = ImVec2(from.x + dx * t0, from.y + dy * t0);
                ImVec2 p1 = ImVec2(from.x + dx * t1, from.y + dy * t1);

                draw->AddLine(p0, p1, IM_COL32(255, 255, 100, 80), 1.0f);
            }

            bool aIsReceiver = (a.Type == TeleporterType::Receiver ||
                a.Type == TeleporterType::TwoWay);

            bool bIsReceiver = (b.Type == TeleporterType::Receiver ||
                b.Type == TeleporterType::TwoWay);

            auto drawArrow = [&](ImVec2 tip, float arrowDx, float arrowDy) {
                float  arrowSize = 6.0f;
                float  perpX = arrowDy, perpY = -arrowDx;

                ImVec2 left = ImVec2(
                    tip.x - arrowDx * arrowSize + perpX * arrowSize * 0.5f,
                    tip.y - arrowDy * arrowSize + perpY * arrowSize * 0.5f);

                ImVec2 right = ImVec2(
                    tip.x - arrowDx * arrowSize - perpX * arrowSize * 0.5f,
                    tip.y - arrowDy * arrowSize - perpY * arrowSize * 0.5f);

                draw->AddTriangleFilled(tip, left, right,
                    IM_COL32(255, 255, 100, 200));
                };

            ImVec2 mid = ImVec2((from.x + to.x) * 0.5f,
                (from.y + to.y) * 0.5f);

            if (bIsReceiver)
            {
                drawArrow({ mid.x + ndx * 4.0f,
                    mid.y + ndy * 4.0f }, ndx, ndy);

                ImVec2 nearB = { to.x - ndx * 12.0f,
                    to.y - ndy * 12.0f };

                drawArrow({ nearB.x + ndx * 4.0f,
                    nearB.y + ndy * 4.0f }, ndx, ndy);
            }

            if (aIsReceiver)
            {
                drawArrow({ mid.x - ndx * 4.0f,
                    mid.y - ndy * 4.0f }, -ndx, -ndy);

                ImVec2 nearA = { from.x + ndx * 12.0f,
                    from.y + ndy * 12.0f };

                drawArrow({ nearA.x - ndx * 4.0f,
                    nearA.y - ndy * 4.0f }, -ndx, -ndy);
            }
        }
    }

    for (int32_t i = 0; i < static_cast<int32_t>(
        m_CachedTeleporters.size()); ++i)
    {
        const auto& teleporter = m_CachedTeleporters[i];

        ImVec2 center = UI_MapHelpers::ToCanvas(teleporter.Position[0],
            teleporter.Position[1], transform);

        bool selected = (selection.Type == MapSelectionType::Teleporter &&
            selection.Index == i);

        ImU32 nodeColor = selected ?
            IM_COL32(255, 255, 160, 255) : IM_COL32(255, 255, 100, 220);

        ImU32 nodeFill = selected ?
            IM_COL32(255, 255, 160, 100) : IM_COL32(255, 255, 100, 50);

        const ShapeType shape = teleporter.ZoneShape.ShapeType;

        if (shape == ShapeType::Cylinder)
        {
            auto up = UI_MapHelpers::Normalize(teleporter.Up);

            std::array<float, 3> ref = { 0.0f, 0.0f, 1.0f };

            if (std::abs(up[2]) > 0.9f)
            {
                ref = { 1.0f, 0.0f, 0.0f };
            }

            auto side1 = UI_MapHelpers::Normalize(
                UI_MapHelpers::Cross(up, ref));

            auto side2 = UI_MapHelpers::Cross(up, side1);

            float radius = teleporter.ZoneShape.Radius;
            float top = teleporter.ZoneShape.Top;
            float bot = teleporter.ZoneShape.Bottom;

            constexpr int kSamples = 16;
            std::vector<ImVec2> pts;

            pts.reserve(kSamples * 2);

            for (int j = 0; j < kSamples; ++j)
            {
                float angle = (j / float(kSamples)) * 6.2832f;

                float cx = std::cos(angle) * radius;
                float cy = std::sin(angle) * radius;

                pts.push_back(UI_MapHelpers::ToCanvas(
                    teleporter.Position[0] + side1[0] * cx +
                    side2[0] * cy + up[0] * top,
                    teleporter.Position[1] + side1[1] * cx +
                    side2[1] * cy + up[1] * top, transform));

                pts.push_back(UI_MapHelpers::ToCanvas(
                    teleporter.Position[0] + side1[0] * cx +
                    side2[0] * cy + up[0] * (-bot),
                    teleporter.Position[1] + side1[1] * cx +
                    side2[1] * cy + up[1] * (-bot), transform));
            }

            UI_MapHelpers::DrawConvexHullFilled(draw, pts, nodeFill);

            UI_MapHelpers::DrawConvexHullDashed(draw, pts, nodeColor,
                selected ? 2.5f : 1.5f);
        }
        else if (shape == ShapeType::Box)
        {
            auto fwd = UI_MapHelpers::Normalize(teleporter.Forward);
            auto up = UI_MapHelpers::Normalize(teleporter.Up);

            auto rgt = UI_MapHelpers::Normalize(
                UI_MapHelpers::Cross(fwd, up));

            float halfW = teleporter.ZoneShape.Radius * 0.5f;
            float halfL = teleporter.ZoneShape.Length * 0.5f;
            float top = teleporter.ZoneShape.Top;
            float bot = teleporter.ZoneShape.Bottom;

            auto project = [&](float fw, float rw, float uw) -> ImVec2 {
                return UI_MapHelpers::ToCanvas(
                    teleporter.Position[0] + fwd[0] * fw +
                    rgt[0] * rw + up[0] * uw,
                    teleporter.Position[1] + fwd[1] * fw +
                    rgt[1] * rw + up[1] * uw, transform);
                };

            std::vector<ImVec2> pts = {
                project(halfL,  halfW,  top),
                project(halfL, -halfW,  top),
                project(-halfL, -halfW,  top),
                project(-halfL,  halfW,  top),
                project(halfL,  halfW, -bot),
                project(halfL, -halfW, -bot),
                project(-halfL, -halfW, -bot),
                project(-halfL,  halfW, -bot),
            };

            UI_MapHelpers::DrawConvexHullFilled(draw, pts, nodeFill);

            UI_MapHelpers::DrawConvexHullDashed(draw, pts, nodeColor,
                selected ? 2.5f : 1.5f);
        }
        else
        {
            draw->AddCircleFilled(center, 6.0f, nodeFill);

            draw->AddCircle(center, 6.0f, nodeColor, 8,
                selected ? 2.5f : 1.5f);
        }

        draw->AddCircleFilled(center, 4.0f, nodeFill);
        draw->AddCircle(center, 4.0f, nodeColor, 8, 1.0f);
    }
}

void UI_Navigation::DrawLifts(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(
        m_CachedLifts.size()); ++i)
    {
        const auto& lift = m_CachedLifts[i];

        ImVec2 center = UI_MapHelpers::ToCanvas(lift.Position[0],
            lift.Position[1], transform);

        bool selected = (selection.Type == MapSelectionType::Lift &&
            selection.Index == i);

        draw->AddCircleFilled(center, 6.0f, selected ?
            IM_COL32(200, 255, 255, 100) : IM_COL32(160, 255, 255, 50));

        draw->AddCircle(center, 6.0f, selected ?
            IM_COL32(200, 255, 255, 255) : IM_COL32(160, 255, 255, 220),
            8, selected ? 2.5f : 1.5f);
    }
}

void UI_Navigation::DrawShields(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(
        m_CachedShields.size()); ++i)
    {
        const auto& shield = m_CachedShields[i];

        ImVec2 center = UI_MapHelpers::ToCanvas(shield.Position[0],
            shield.Position[1], transform);

        bool selected = (selection.Type == MapSelectionType::Shield &&
            selection.Index == i);

        draw->AddCircleFilled(center, 6.0f, selected ?
            IM_COL32(220, 160, 255, 100) : IM_COL32(180, 100, 255, 50));

        draw->AddCircle(center, 6.0f, selected ?
            IM_COL32(220, 160, 255, 255) : IM_COL32(180, 100, 255, 220),
            8, selected ? 2.5f : 1.5f);
    }
}

void UI_Navigation::DrawObjectiveSpawns(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(
        m_CachedObjectiveSpawns.size()); ++i)
    {
        const auto& spawn = m_CachedObjectiveSpawns[i];

        ImVec2 center = UI_MapHelpers::ToCanvas(spawn.Position[0],
            spawn.Position[1], transform);

        bool selected = (selection.Type ==
            MapSelectionType::ObjectiveSpawn && selection.Index == i);

        ImU32 color = TeamToColor::TeamColorU32(spawn.Team,
            selected ? 255 : 220);

        ImU32 fillColor = TeamToColor::TeamColorU32(spawn.Team,
            selected ? 120 : 60);

        const ShapeType shape = spawn.ZoneShape.ShapeType;

        if (shape == ShapeType::Cylinder)
        {
            auto up = UI_MapHelpers::Normalize(spawn.Up);
            std::array<float, 3> ref = { 0.0f, 0.0f, 1.0f };

            if (std::abs(up[2]) > 0.9f)
            {
                ref = { 1.0f, 0.0f, 0.0f };
            }

            auto side1 = UI_MapHelpers::Normalize(
                UI_MapHelpers::Cross(up, ref));

            auto side2 = UI_MapHelpers::Cross(up, side1);

            float radius = spawn.ZoneShape.Radius;
            float top = spawn.ZoneShape.Top;
            float bot = spawn.ZoneShape.Bottom;

            constexpr int kSamples = 16;
            std::vector<ImVec2> pts;

            pts.reserve(kSamples * 2);

            for (int j = 0; j < kSamples; ++j)
            {
                float angle = (j / float(kSamples)) * 6.2832f;

                float cx = std::cos(angle) * radius;
                float cy = std::sin(angle) * radius;

                pts.push_back(UI_MapHelpers::ToCanvas(
                    spawn.Position[0] + side1[0] * cx +
                    side2[0] * cy + up[0] * top,
                    spawn.Position[1] + side1[1] * cx +
                    side2[1] * cy + up[1] * top, transform));

                pts.push_back(UI_MapHelpers::ToCanvas(
                    spawn.Position[0] + side1[0] * cx +
                    side2[0] * cy + up[0] * (-bot),
                    spawn.Position[1] + side1[1] * cx +
                    side2[1] * cy + up[1] * (-bot), transform));
            }

            UI_MapHelpers::DrawConvexHullFilled(draw, pts, fillColor);

            UI_MapHelpers::DrawConvexHullDashed(draw, pts, color,
                selected ? 2.5f : 1.5f);
        }
        else if (shape == ShapeType::Box)
        {
            auto fwd = UI_MapHelpers::Normalize(spawn.Forward);
            auto up = UI_MapHelpers::Normalize(spawn.Up);

            auto rgt = UI_MapHelpers::Normalize(
                UI_MapHelpers::Cross(fwd, up));

            float halfW = spawn.ZoneShape.Radius * 0.5f;
            float halfL = spawn.ZoneShape.Length * 0.5f;
            float top = spawn.ZoneShape.Top;
            float bot = spawn.ZoneShape.Bottom;

            auto project = [&](float fw, float rw, float uw) -> ImVec2 {
                return UI_MapHelpers::ToCanvas(
                    spawn.Position[0] + fwd[0] * fw +
                    rgt[0] * rw + up[0] * uw,
                    spawn.Position[1] + fwd[1] * fw +
                    rgt[1] * rw + up[1] * uw, transform);
                };

            std::vector<ImVec2> pts = {
                project(halfL,  halfW,  top),
                project(halfL, -halfW,  top),
                project(-halfL, -halfW,  top),
                project(-halfL,  halfW,  top),
                project(halfL,  halfW, -bot),
                project(halfL, -halfW, -bot),
                project(-halfL, -halfW, -bot),
                project(-halfL,  halfW, -bot),
            };

            UI_MapHelpers::DrawConvexHullFilled(draw, pts, fillColor);

            UI_MapHelpers::DrawConvexHullDashed(draw, pts, color,
                selected ? 2.5f : 1.5f);
        }
        else
        {
            float r = 7.0f;

            ImVec2 top = { center.x,       center.y - r };
            ImVec2 right = { center.x + r,   center.y };
            ImVec2 bot = { center.x,       center.y + r };
            ImVec2 left = { center.x - r,   center.y };

            draw->AddQuadFilled(top, right, bot, left, fillColor);

            draw->AddQuad(top, right, bot, left, color,
                selected ? 2.5f : 1.5f);
        }

        draw->AddCircleFilled(center, 4.0f, fillColor);
        draw->AddCircle(center, 4.0f, color, 8, 1.0f);
    }
}

void UI_Navigation::DrawObjectives(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(
        m_CachedObjectives.size()); ++i)
    {
        const auto& obj = m_CachedObjectives[i];

        ImVec2 center = UI_MapHelpers::ToCanvas(obj.Position[0],
            obj.Position[1], transform);

        float r = 6.0f;

        bool selected = (selection.Type == MapSelectionType::Objective &&
            selection.Index == i);

        uint8_t alpha = selected ? 255 : obj.IsEquipped ? 220 : 120;
        ImU32 color = TeamToColor::TeamColorU32(obj.Team, alpha);

        draw->AddLine({ center.x - r, center.y }, { center.x + r,
            center.y }, color, selected ? 3.0f : 2.0f);

        draw->AddLine({ center.x, center.y - r }, { center.x,
            center.y + r }, color, selected ? 3.0f : 2.0f);

        draw->AddCircle(center, r * 0.4f, color, 8, selected ? 2.5f : 1.5f);
    }
}

void UI_Navigation::DrawDestructibles(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(
        m_CachedDestructibles.size()); ++i)
    {
        const auto& dest = m_CachedDestructibles[i];

        ImVec2 center = UI_MapHelpers::ToCanvas(dest.Position[0],
            dest.Position[1], transform);

        bool selected = (selection.Type ==
            MapSelectionType::Destructible && selection.Index == i);

        ImU32 color = selected ?
            IM_COL32(255, 80, 80, 255) : IM_COL32(220, 60, 60, 220);

        ImU32 fillColor = selected ?
            IM_COL32(255, 80, 80, 100) : IM_COL32(220, 60, 60, 50);

        if (dest.Type == DestructibleType::Explosive)
        {
            float r = 7.0f;

            ImVec2 p0 = { center.x,                   center.y - r };
            ImVec2 p1 = { center.x + r * 0.866f,      center.y + r * 0.5f };
            ImVec2 p2 = { center.x - r * 0.866f,      center.y + r * 0.5f };

            draw->AddTriangleFilled(p0, p1, p2, fillColor);
            draw->AddTriangle(p0, p1, p2, color, selected ? 2.5f : 1.5f);
        }
        else if (dest.Type == DestructibleType::Pallet)
        {
            float r = 5.0f;

            draw->AddRectFilled({ center.x - r, center.y - r },
                { center.x + r, center.y + r }, fillColor);

            draw->AddRect({ center.x - r, center.y - r },
                { center.x + r, center.y + r }, color,
                0.0f, 0, selected ? 2.5f : 1.5f);
        }
        else
        {
            draw->AddCircleFilled(center, 6.0f, fillColor);
            draw->AddCircle(center, 6.0f, color, 8, selected ? 2.5f : 1.5f);
        }
    }
}

void UI_Navigation::DrawKillZones(ImDrawList* draw,
    const MapTransform& transform) const
{
    if (!m_HasZones) return;

    for (const auto& vol : m_CachedZones.KillZones)
    {
        this->DrawTriggerVolume(draw, transform, vol,
            IM_COL32(220, 40, 40, 200), IM_COL32(220, 40, 40, 30));
    }
}

void UI_Navigation::DrawSafeZones(ImDrawList* draw,
    const MapTransform& transform) const
{
    if (!m_HasZones) return;

    for (const auto& vol : m_CachedZones.SafeZones)
    {
        this->DrawTriggerVolume(draw, transform, vol,
            IM_COL32(40, 220, 40, 200), IM_COL32(40, 220, 40, 30));
    }
}

void UI_Navigation::DrawTriggerVolume(ImDrawList* draw,
    const MapTransform& transform, const ScnrTriggerVolume& vol,
    ImU32 color, ImU32 fillColor) const
{
    auto fwd = UI_MapHelpers::Normalize({
        vol.Forward[0], vol.Forward[1], vol.Forward[2] });

    auto up = UI_MapHelpers::Normalize({
        vol.Up[0], vol.Up[1], vol.Up[2] });

    auto rgt = UI_MapHelpers::Normalize(
        UI_MapHelpers::Cross(fwd, up));

    float ex = vol.Extents[0];
    float ey = vol.Extents[1];
    float ez = vol.Extents[2];

    const float sx[8] = { -1,+1,-1,+1,-1,+1,-1,+1 };
    const float sy[8] = { -1,-1,+1,+1,-1,-1,+1,+1 };
    const float sz[8] = { -1,-1,-1,-1,+1,+1,+1,+1 };

    std::vector<ImVec2> pts;
    pts.reserve(8);

    for (int i = 0; i < 8; ++i)
    {
        float wx = vol.Position[0] + fwd[0] * ex * sx[i]
            + rgt[0] * ey * sy[i] + up[0] * ez * sz[i];

        float wy = vol.Position[1] + fwd[1] * ex * sx[i]
            + rgt[1] * ey * sy[i] + up[1] * ez * sz[i];

        pts.push_back(UI_MapHelpers::ToCanvas(wx, wy, transform));
    }

    UI_MapHelpers::DrawConvexHullFilled(draw, pts, fillColor);
    UI_MapHelpers::DrawConvexHullDashed(draw, pts, color, 1.5f);
}

bool UI_Navigation::PointInTriggerVolumeXY(const ImVec2& mouse,
    const ScnrTriggerVolume& vol, const MapTransform& transform) const
{
    // We project the mouse from screen-space to world-space XY.
    float wx = (mouse.x - transform.CanvasPos.x - transform.OffsetX)
        / transform.Scale + transform.WorldMinX;

    float wy = -((mouse.y - transform.CanvasPos.y - transform.OffsetY)
        / transform.Scale - transform.WorldMinY);

    // AABB test in world-space XY — sufficient for most volumes
    // axis-aligned like those of Forge World.
    float minX = vol.Position[0] - vol.Extents[0];
    float maxX = vol.Position[0] + vol.Extents[0];
    float minY = vol.Position[1] - vol.Extents[1];
    float maxY = vol.Position[1] + vol.Extents[1];

    return wx >= minX && wx <= maxX && wy >= minY && wy <= maxY;
}

float UI_Navigation::VolumeScreenRadius(const ScnrTriggerVolume& vol,
    const MapTransform& transform) const
{
    // Approximate radio in screen-space for the candidate ranking.
    float worldRadius = (std::max)(vol.Extents[0], vol.Extents[1]);
    return worldRadius * transform.Scale;
}