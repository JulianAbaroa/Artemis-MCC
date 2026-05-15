#include "pch.h"

// Header.
#include "UI_Navigation.h"

// --- States ---

#include "Core/States/Domain/Navigation/State_Navigation.h"

#include <algorithm>
#include <limits>
#include <cmath>

void UI_Navigation::FetchState()
{
    m_CachedGraph = m_State_Navigation.GetNavigationGraph();
    m_CachedRawGraph = m_State_Navigation.GetRawNavigationGraph();
    m_CachedObstacles = m_State_Navigation.GetActiveObstacles();
    m_CachedSpawns = m_State_Navigation.GetActiveSpawns();
    m_CachedTeleporters = m_State_Navigation.GetActiveTeleporters();
    m_CachedLifts = m_State_Navigation.GetActiveLifts();
    m_CachedShields = m_State_Navigation.GetActiveShields();
    m_CachedObjectiveSpawns = m_State_Navigation.GetActiveObjectiveSpawns();
    m_CachedObjectives = m_State_Navigation.GetActiveObjectives();
    m_CachedDestructibles = m_State_Navigation.GetActiveDestructibles();
}

void UI_Navigation::Cleanup()
{
    m_ShowAllClusters = false;

    m_CachedGraph.clear();
    m_CachedRawGraph.clear();
    m_CachedObstacles.clear();
    m_CachedSpawns.clear();
    m_CachedTeleporters.clear();
    m_CachedLifts.clear();
    m_CachedShields.clear();
    m_CachedObjectiveSpawns.clear();
    m_CachedObjectives.clear();
    m_CachedDestructibles.clear();
}

void UI_Navigation::Draw(const MapTransform& transform,
    const MapSelection& selection)
{
    bool showAllClusters = m_ShowAllClusters;
    if (ImGui::Checkbox("Show all clusters", &showAllClusters))
    {
        m_ShowAllClusters = showAllClusters;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    this->DrawSelectionPanel(selection);
}

void UI_Navigation::DrawLayers(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection,
    uint32_t visibleLayers) const
{
    auto isVisible = [&](MapLayer layer) {
        return (visibleLayers & static_cast<uint32_t>(layer)) != 0;
        };

    if (isVisible(MapLayer::Clusters))
        this->DrawClusters(draw, transform, selection);

    if (isVisible(MapLayer::Obstacles))
        this->DrawObstacles(draw, transform, selection);

    if (isVisible(MapLayer::Spawns))
        this->DrawSpawns(draw, transform, selection);

    if (isVisible(MapLayer::Teleporters))
        this->DrawTeleporters(draw, transform, selection);

    if (isVisible(MapLayer::Lifts))
        this->DrawLifts(draw, transform, selection);

    if (isVisible(MapLayer::Shields))
        this->DrawShields(draw, transform, selection);

    if (isVisible(MapLayer::ObjectiveSpawns))
        this->DrawObjectiveSpawns(draw, transform, selection);

    if (isVisible(MapLayer::Objectives))
        this->DrawObjectives(draw, transform, selection);

    if (isVisible(MapLayer::Destructibles))
        this->DrawDestructibles(draw, transform, selection);
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
        if (i >= 0 && i < static_cast<int32_t>(m_CachedGraph.size()))
            this->DrawSelectionCluster(m_CachedGraph[i]);
        break;

    case MapSelectionType::Obstacle:
        if (i >= 0 && i < static_cast<int32_t>(m_CachedObstacles.size()))
            this->DrawSelectionObstacle(m_CachedObstacles[i]);
        break;

    case MapSelectionType::Spawn:
        if (i >= 0 && i < static_cast<int32_t>(m_CachedSpawns.size()))
            this->DrawSelectionSpawn(m_CachedSpawns[i]);
        break;

    case MapSelectionType::Teleporter:
        if (i >= 0 && i < static_cast<int32_t>(m_CachedTeleporters.size()))
            this->DrawSelectionTeleporter(m_CachedTeleporters[i]);
        break;

    case MapSelectionType::Lift:
        if (i >= 0 && i < static_cast<int32_t>(m_CachedLifts.size()))
            this->DrawSelectionLift(m_CachedLifts[i]);
        break;

    case MapSelectionType::Shield:
        if (i >= 0 && i < static_cast<int32_t>(m_CachedShields.size()))
            this->DrawSelectionShield(m_CachedShields[i]);
        break;

    case MapSelectionType::ObjectiveSpawn:
        if (i >= 0 && i < static_cast<int32_t>(m_CachedObjectiveSpawns.size()))
            this->DrawSelectionObjectiveSpawn(m_CachedObjectiveSpawns[i]);
        break;

    case MapSelectionType::Objective:
        if (i >= 0 && i < static_cast<int32_t>(m_CachedObjectives.size()))
            this->DrawSelectionObjective(m_CachedObjectives[i]);
        break;

    case MapSelectionType::Destructible:
        if (i >= 0 && i < static_cast<int32_t>(m_CachedDestructibles.size()))
            this->DrawSelectionDestructible(m_CachedDestructibles[i]);
        break;

    default:
        break;
    }

    ImGui::Unindent(5.0f);
}

void UI_Navigation::CollectCandidates(const MapTransform& transform,
    std::vector<MapCandidate>& candidates, uint32_t visibleLayers) const
{
    auto isVisible = [&](MapLayer layer) {
        return (visibleLayers & static_cast<uint32_t>(layer)) != 0;
        };

    const ImVec2 mouse = ImGui::GetIO().MousePos;

    auto tryAdd = [&](MapSelectionType type, int32_t index,
        float wx, float wy, float screenRadius)
        {
            ImVec2 center = UI_MapHelpers::ToCanvas(wx, wy, transform);
            float dx = mouse.x - center.x;
            float dy = mouse.y - center.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= screenRadius)
                candidates.push_back({ type, index, screenRadius });
        };

    if (isVisible(MapLayer::Clusters))
    {
        const std::vector<AINavigationCluster>& graph =
            m_ShowAllClusters ? m_CachedRawGraph : m_CachedGraph;

        for (int32_t i = 0; i < static_cast<int32_t>(graph.size()); ++i)
        {
            const auto& c = graph[i];
            ImVec2 pMin = UI_MapHelpers::ToCanvas(
                c.BoundsMin[0], c.BoundsMin[1], transform);
            ImVec2 pMax = UI_MapHelpers::ToCanvas(
                c.BoundsMax[0], c.BoundsMax[1], transform);

            if (pMin.x > pMax.x) std::swap(pMin.x, pMax.x);
            if (pMin.y > pMax.y) std::swap(pMin.y, pMax.y);

            if (mouse.x >= pMin.x && mouse.x <= pMax.x &&
                mouse.y >= pMin.y && mouse.y <= pMax.y)
            {
                float w = pMax.x - pMin.x;
                float h = pMax.y - pMin.y;
                candidates.push_back({ MapSelectionType::Cluster,
                    i, std::sqrt(w * w + h * h) * 0.5f });
            }
        }
    }

    if (isVisible(MapLayer::Obstacles))
    {
        for (int32_t i = 0;
            i < static_cast<int32_t>(m_CachedObstacles.size()); ++i)
        {
            const auto& obs = m_CachedObstacles[i];
            float radius = (std::max)(obs.BoundingRadius * transform.Scale, 3.0f);
            tryAdd(MapSelectionType::Obstacle, i,
                obs.Position[0], obs.Position[1], radius);
        }
    }

    if (isVisible(MapLayer::Spawns))
    {
        for (int32_t i = 0;
            i < static_cast<int32_t>(m_CachedSpawns.size()); ++i)
        {
            const auto& spawn = m_CachedSpawns[i];
            tryAdd(MapSelectionType::Spawn, i,
                spawn.Position[0], spawn.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::Teleporters))
    {
        for (int32_t i = 0;
            i < static_cast<int32_t>(m_CachedTeleporters.size()); ++i)
        {
            const auto& tele = m_CachedTeleporters[i];
            tryAdd(MapSelectionType::Teleporter, i,
                tele.Position[0], tele.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::Lifts))
    {
        for (int32_t i = 0;
            i < static_cast<int32_t>(m_CachedLifts.size()); ++i)
        {
            const auto& lift = m_CachedLifts[i];
            tryAdd(MapSelectionType::Lift, i,
                lift.Position[0], lift.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::Shields))
    {
        for (int32_t i = 0;
            i < static_cast<int32_t>(m_CachedShields.size()); ++i)
        {
            const auto& shield = m_CachedShields[i];
            tryAdd(MapSelectionType::Shield, i,
                shield.Position[0], shield.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::ObjectiveSpawns))
    {
        for (int32_t i = 0;
            i < static_cast<int32_t>(m_CachedObjectiveSpawns.size()); ++i)
        {
            const auto& spawn = m_CachedObjectiveSpawns[i];
            tryAdd(MapSelectionType::ObjectiveSpawn, i,
                spawn.Position[0], spawn.Position[1], 7.0f);
        }
    }

    if (isVisible(MapLayer::Objectives))
    {
        for (int32_t i = 0;
            i < static_cast<int32_t>(m_CachedObjectives.size()); ++i)
        {
            const auto& obj = m_CachedObjectives[i];
            tryAdd(MapSelectionType::Objective, i,
                obj.Position[0], obj.Position[1], 6.0f);
        }
    }

    if (isVisible(MapLayer::Destructibles))
    {
        for (int32_t i = 0;
            i < static_cast<int32_t>(m_CachedDestructibles.size()); ++i)
        {
            const auto& dest = m_CachedDestructibles[i];
            tryAdd(MapSelectionType::Destructible, i,
                dest.Position[0], dest.Position[1], 6.0f);
        }
    }
}

void UI_Navigation::GetWorldBounds(float& outMinX, float& outMinY,
    float& outMaxX, float& outMaxY) const
{
    const std::vector<AINavigationCluster>& graph =
        m_ShowAllClusters ? m_CachedRawGraph : m_CachedGraph;

    for (const auto& c : graph)
    {
        outMinX = (std::min)(outMinX, c.BoundsMin[0]);
        outMinY = (std::min)(outMinY, c.BoundsMin[1]);
        outMaxX = (std::max)(outMaxX, c.BoundsMax[0]);
        outMaxY = (std::max)(outMaxY, c.BoundsMax[1]);
    }
}

// --- Draw layers ---

void UI_Navigation::DrawClusters(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    const std::vector<AINavigationCluster>& graph =
        m_ShowAllClusters ? m_CachedRawGraph : m_CachedGraph;

    for (int32_t i = 0; i < static_cast<int32_t>(graph.size()); ++i)
    {
        const auto& c = graph[i];
        ImVec2 pMin = UI_MapHelpers::ToCanvas(c.BoundsMin[0], c.BoundsMin[1], transform);
        ImVec2 pMax = UI_MapHelpers::ToCanvas(c.BoundsMax[0], c.BoundsMax[1], transform);

        if (pMin.x > pMax.x) std::swap(pMin.x, pMax.x);
        if (pMin.y > pMax.y) std::swap(pMin.y, pMax.y);

        bool isSelected = (selection.Type == MapSelectionType::Cluster &&
            selection.Index == i);

        ImU32 fillColor = isSelected ? IM_COL32(80, 200, 80, 100) : IM_COL32(40, 80, 120, 50);
        ImU32 borderColor = isSelected ? IM_COL32(140, 255, 140, 255) : IM_COL32(80, 160, 220, 180);

        draw->AddRectFilled(pMin, pMax, fillColor);
        draw->AddRect(pMin, pMax, borderColor, 0.0f, 0,
            isSelected ? 2.5f : 1.0f);

        float screenW = pMax.x - pMin.x;
        float screenH = pMax.y - pMin.y;
        if (screenW > 16.0f && screenH > 10.0f)
        {
            ImVec2 center = UI_MapHelpers::ToCanvas(c.Center[0], c.Center[1], transform);
            char label[16];
            snprintf(label, sizeof(label), "%d", c.ClusterIndex);
            draw->AddText(
                ImVec2(center.x - 5.0f, center.y - 7.0f),
                isSelected ? IM_COL32(140, 255, 140, 255)
                : IM_COL32(180, 180, 180, 200),
                label);
        }
    }

    for (const auto& c : graph)
    {
        for (const auto& link : c.Links)
        {
            if (link.SbspIndex != c.SbspIndex) continue;
            ImVec2 from = UI_MapHelpers::ToCanvas(
                link.Centroid[0], link.Centroid[1], transform);
            ImVec2 to = UI_MapHelpers::ToCanvas(
                c.Center[0], c.Center[1], transform);
            draw->AddLine(from, to, IM_COL32(100, 100, 50, 100), 1.0f);
        }
    }
}

void UI_Navigation::DrawObstacles(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(m_CachedObstacles.size()); ++i)
    {
        const auto& obs = m_CachedObstacles[i];
        ImVec2 center = UI_MapHelpers::ToCanvas(
            obs.Position[0], obs.Position[1], transform);
        float radius = (std::max)(obs.BoundingRadius * transform.Scale, 3.0f);
        bool selected = (selection.Type == MapSelectionType::Obstacle &&
            selection.Index == i);

        draw->AddCircle(center, radius,
            selected ? IM_COL32(255, 200, 80, 255) : IM_COL32(255, 140, 0, 180),
            12, selected ? 2.5f : 1.5f);

        draw->AddCircleFilled(center, radius,
            selected ? IM_COL32(255, 200, 80, 60) : IM_COL32(255, 140, 0, 30));
    }
}

void UI_Navigation::DrawSpawns(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(m_CachedSpawns.size()); ++i)
    {
        const auto& spawn = m_CachedSpawns[i];
        ImVec2 center = UI_MapHelpers::ToCanvas(
            spawn.Position[0], spawn.Position[1], transform);
        float radius = 6.0f;
        bool selected = (selection.Type == MapSelectionType::Spawn &&
            selection.Index == i);

        ImU32 color = TeamToColor::TeamColor(spawn.Team, selected ? 255 : 220);
        ImU32 fillColor = TeamToColor::TeamColor(spawn.Team, selected ? 120 : 60);

        draw->AddCircleFilled(center, radius, fillColor);
        draw->AddCircle(center, radius, color, 8, selected ? 2.5f : 1.5f);

        ImVec2 tip = ImVec2(
            center.x + spawn.Forward[0] * radius * 2.0f,
            center.y - spawn.Forward[1] * radius * 2.0f);
        draw->AddLine(center, tip, color, 1.5f);
    }
}

void UI_Navigation::DrawTeleporters(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    // Connection lines.
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

            ImVec2 from = UI_MapHelpers::ToCanvas(
                a.Position[0], a.Position[1], transform);
            ImVec2 to = UI_MapHelpers::ToCanvas(
                b.Position[0], b.Position[1], transform);

            float dx = to.x - from.x;
            float dy = to.y - from.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < 0.001f) continue;

            float ndx = dx / len;
            float ndy = dy / len;

            float dash = 6.0f, gap = 4.0f, step = dash + gap;
            int   segs = static_cast<int>(len / step);

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

            auto drawArrow = [&](ImVec2 tip, float arrowDx, float arrowDy)
                {
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
                drawArrow({ mid.x + ndx * 4.0f, mid.y + ndy * 4.0f }, ndx, ndy);
                ImVec2 nearB = { to.x - ndx * 12.0f, to.y - ndy * 12.0f };
                drawArrow({ nearB.x + ndx * 4.0f, nearB.y + ndy * 4.0f }, ndx, ndy);
            }
            if (aIsReceiver)
            {
                drawArrow({ mid.x - ndx * 4.0f, mid.y - ndy * 4.0f }, -ndx, -ndy);
                ImVec2 nearA = { from.x + ndx * 12.0f, from.y + ndy * 12.0f };
                drawArrow({ nearA.x - ndx * 4.0f, nearA.y - ndy * 4.0f }, -ndx, -ndy);
            }
        }
    }

    // Nodes.
    for (int32_t i = 0; i < static_cast<int32_t>(m_CachedTeleporters.size()); ++i)
    {
        const auto& tele = m_CachedTeleporters[i];
        ImVec2 center = UI_MapHelpers::ToCanvas(
            tele.Position[0], tele.Position[1], transform);
        bool selected = (selection.Type == MapSelectionType::Teleporter &&
            selection.Index == i);

        ImU32 nodeColor = selected ? IM_COL32(255, 255, 160, 255)
            : IM_COL32(255, 255, 100, 220);
        ImU32 nodeFill = selected ? IM_COL32(255, 255, 160, 100)
            : IM_COL32(255, 255, 100, 50);

        const ShapeType shape = tele.ZoneShape.ShapeType;

        if (shape == ShapeType::Cylinder)
        {
            auto up = UI_MapHelpers::Normalize(tele.Up);
            std::array<float, 3> ref = { 0.0f, 0.0f, 1.0f };
            if (std::abs(up[2]) > 0.9f) ref = { 1.0f, 0.0f, 0.0f };
            auto side1 = UI_MapHelpers::Normalize(UI_MapHelpers::Cross(up, ref));
            auto side2 = UI_MapHelpers::Cross(up, side1);

            float radius = tele.ZoneShape.Radius;
            float top = tele.ZoneShape.Top;
            float bot = tele.ZoneShape.Bottom;

            constexpr int kSamples = 16;
            std::vector<ImVec2> pts;
            pts.reserve(kSamples * 2);

            for (int j = 0; j < kSamples; ++j)
            {
                float angle = (j / float(kSamples)) * 6.2832f;
                float cx = std::cos(angle) * radius;
                float cy = std::sin(angle) * radius;

                pts.push_back(UI_MapHelpers::ToCanvas(
                    tele.Position[0] + side1[0] * cx + side2[0] * cy + up[0] * top,
                    tele.Position[1] + side1[1] * cx + side2[1] * cy + up[1] * top,
                    transform));

                pts.push_back(UI_MapHelpers::ToCanvas(
                    tele.Position[0] + side1[0] * cx + side2[0] * cy + up[0] * (-bot),
                    tele.Position[1] + side1[1] * cx + side2[1] * cy + up[1] * (-bot),
                    transform));
            }

            UI_MapHelpers::DrawConvexHullFilled(draw, pts, nodeFill);
            UI_MapHelpers::DrawConvexHullDashed(draw, pts, nodeColor,
                selected ? 2.5f : 1.5f);
        }
        else if (shape == ShapeType::Box)
        {
            auto fwd = UI_MapHelpers::Normalize(tele.Forward);
            auto up = UI_MapHelpers::Normalize(tele.Up);
            auto rgt = UI_MapHelpers::Normalize(UI_MapHelpers::Cross(fwd, up));

            float halfW = tele.ZoneShape.Radius * 0.5f;
            float halfL = tele.ZoneShape.Length * 0.5f;
            float top = tele.ZoneShape.Top;
            float bot = tele.ZoneShape.Bottom;

            auto project = [&](float fw, float rw, float uw) -> ImVec2
                {
                    return UI_MapHelpers::ToCanvas(
                        tele.Position[0] + fwd[0] * fw + rgt[0] * rw + up[0] * uw,
                        tele.Position[1] + fwd[1] * fw + rgt[1] * rw + up[1] * uw,
                        transform);
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
    for (int32_t i = 0; i < static_cast<int32_t>(m_CachedLifts.size()); ++i)
    {
        const auto& lift = m_CachedLifts[i];
        ImVec2 center = UI_MapHelpers::ToCanvas(
            lift.Position[0], lift.Position[1], transform);
        bool selected = (selection.Type == MapSelectionType::Lift &&
            selection.Index == i);

        draw->AddCircleFilled(center, 6.0f,
            selected ? IM_COL32(200, 255, 255, 100)
            : IM_COL32(160, 255, 255, 50));
        draw->AddCircle(center, 6.0f,
            selected ? IM_COL32(200, 255, 255, 255)
            : IM_COL32(160, 255, 255, 220),
            8, selected ? 2.5f : 1.5f);
    }
}

void UI_Navigation::DrawShields(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(m_CachedShields.size()); ++i)
    {
        const auto& shield = m_CachedShields[i];
        ImVec2 center = UI_MapHelpers::ToCanvas(
            shield.Position[0], shield.Position[1], transform);
        bool selected = (selection.Type == MapSelectionType::Shield &&
            selection.Index == i);

        draw->AddCircleFilled(center, 6.0f,
            selected ? IM_COL32(220, 160, 255, 100)
            : IM_COL32(180, 100, 255, 50));
        draw->AddCircle(center, 6.0f,
            selected ? IM_COL32(220, 160, 255, 255)
            : IM_COL32(180, 100, 255, 220),
            8, selected ? 2.5f : 1.5f);
    }
}

void UI_Navigation::DrawObjectiveSpawns(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(m_CachedObjectiveSpawns.size()); ++i)
    {
        const auto& spawn = m_CachedObjectiveSpawns[i];
        ImVec2 center = UI_MapHelpers::ToCanvas(
            spawn.Position[0], spawn.Position[1], transform);
        bool selected = (selection.Type == MapSelectionType::ObjectiveSpawn &&
            selection.Index == i);

        ImU32 color = TeamToColor::TeamColor(spawn.Team, selected ? 255 : 220);
        ImU32 fillColor = TeamToColor::TeamColor(spawn.Team, selected ? 120 : 60);

        const ShapeType shape = spawn.ZoneShape.ShapeType;

        if (shape == ShapeType::Cylinder)
        {
            auto up = UI_MapHelpers::Normalize(spawn.Up);
            std::array<float, 3> ref = { 0.0f, 0.0f, 1.0f };
            if (std::abs(up[2]) > 0.9f) ref = { 1.0f, 0.0f, 0.0f };
            auto side1 = UI_MapHelpers::Normalize(UI_MapHelpers::Cross(up, ref));
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
                    spawn.Position[0] + side1[0] * cx + side2[0] * cy + up[0] * top,
                    spawn.Position[1] + side1[1] * cx + side2[1] * cy + up[1] * top,
                    transform));
                pts.push_back(UI_MapHelpers::ToCanvas(
                    spawn.Position[0] + side1[0] * cx + side2[0] * cy + up[0] * (-bot),
                    spawn.Position[1] + side1[1] * cx + side2[1] * cy + up[1] * (-bot),
                    transform));
            }

            UI_MapHelpers::DrawConvexHullFilled(draw, pts, fillColor);
            UI_MapHelpers::DrawConvexHullDashed(draw, pts, color,
                selected ? 2.5f : 1.5f);
        }
        else if (shape == ShapeType::Box)
        {
            auto fwd = UI_MapHelpers::Normalize(spawn.Forward);
            auto up = UI_MapHelpers::Normalize(spawn.Up);
            auto rgt = UI_MapHelpers::Normalize(UI_MapHelpers::Cross(fwd, up));

            float halfW = spawn.ZoneShape.Radius * 0.5f;
            float halfL = spawn.ZoneShape.Length * 0.5f;
            float top = spawn.ZoneShape.Top;
            float bot = spawn.ZoneShape.Bottom;

            auto project = [&](float fw, float rw, float uw) -> ImVec2
                {
                    return UI_MapHelpers::ToCanvas(
                        spawn.Position[0] + fwd[0] * fw + rgt[0] * rw + up[0] * uw,
                        spawn.Position[1] + fwd[1] * fw + rgt[1] * rw + up[1] * uw,
                        transform);
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
    for (int32_t i = 0; i < static_cast<int32_t>(m_CachedObjectives.size()); ++i)
    {
        const auto& obj = m_CachedObjectives[i];
        ImVec2 center = UI_MapHelpers::ToCanvas(
            obj.Position[0], obj.Position[1], transform);
        float r = 6.0f;
        bool selected = (selection.Type == MapSelectionType::Objective &&
            selection.Index == i);

        uint8_t alpha = selected ? 255 : obj.IsEquipped ? 220 : 120;
        ImU32 color = TeamToColor::TeamColor(obj.Team, alpha);

        draw->AddLine({ center.x - r, center.y },
            { center.x + r, center.y }, color, selected ? 3.0f : 2.0f);
        draw->AddLine({ center.x, center.y - r },
            { center.x, center.y + r }, color, selected ? 3.0f : 2.0f);
        draw->AddCircle(center, r * 0.4f, color, 8, selected ? 2.5f : 1.5f);
    }
}

void UI_Navigation::DrawDestructibles(ImDrawList* draw,
    const MapTransform& transform, const MapSelection& selection) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(m_CachedDestructibles.size()); ++i)
    {
        const auto& dest = m_CachedDestructibles[i];
        ImVec2 center = UI_MapHelpers::ToCanvas(
            dest.Position[0], dest.Position[1], transform);
        bool selected = (selection.Type == MapSelectionType::Destructible &&
            selection.Index == i);

        ImU32 color = selected ? IM_COL32(255, 80, 80, 255) : IM_COL32(220, 60, 60, 220);
        ImU32 fillColor = selected ? IM_COL32(255, 80, 80, 100) : IM_COL32(220, 60, 60, 50);

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

// --- Selection panel ---

void UI_Navigation::DrawSelectionCluster(const AINavigationCluster& c) const
{
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Cluster");
    ImGui::Separator();
    ImGui::Text("Index:  %d  (SBSP %d)", c.ClusterIndex, c.SbspIndex);
    ImGui::Text("Center: %.1f, %.1f, %.1f",
        c.Center[0], c.Center[1], c.Center[2]);
    ImGui::Text("Size:   %.1f x %.1f x %.1f",
        c.BoundsMax[0] - c.BoundsMin[0],
        c.BoundsMax[1] - c.BoundsMin[1],
        c.BoundsMax[2] - c.BoundsMin[2]);
    ImGui::Text("Links:  %d", static_cast<int>(c.Links.size()));

    ImGui::Spacing();
    ImGui::TextDisabled("Neighbors:");
    for (const auto& link : c.Links)
    {
        bool isCross = link.SbspIndex != c.SbspIndex;
        ImGui::TextColored(
            isCross ? ImVec4(1.0f, 0.8f, 0.4f, 1.0f)
            : ImVec4(0.6f, 1.0f, 0.6f, 1.0f),
            "  [%s] SBSP %d -> Cluster %d  (r=%.1f)",
            isCross ? "SEAM  " : "PORTAL",
            link.SbspIndex, link.ClusterIndex, link.PassRadius);
    }
}

void UI_Navigation::DrawSelectionObstacle(const ActiveObstacle& obs) const
{
    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.0f, 1.0f), "Obstacle");
    ImGui::Separator();
    ImGui::Text("%s", obs.TagName.c_str());
    ImGui::Text("Position:         %.2f, %.2f, %.2f",
        obs.Position[0], obs.Position[1], obs.Position[2]);
    ImGui::Text("Forward:          %.2f, %.2f, %.2f",
        obs.Forward[0], obs.Forward[1], obs.Forward[2]);
    ImGui::Text("Up:               %.2f, %.2f, %.2f",
        obs.Up[0], obs.Up[1], obs.Up[2]);
    ImGui::Text("Linear Velocity:  %.2f, %.2f, %.2f",
        obs.LinearVelocity[0], obs.LinearVelocity[1], obs.LinearVelocity[2]);
    ImGui::Text("Angular Velocity: %.2f, %.2f, %.2f",
        obs.AngularVelocity[0], obs.AngularVelocity[1], obs.AngularVelocity[2]);
    ImGui::Text("Radius:           %.2f", obs.BoundingRadius);
}

void UI_Navigation::DrawSelectionSpawn(const ActiveSpawn& spawn) const
{
    const char* typeStr =
        spawn.Type == SpawnType::Initial ? "Initial Spawn" :
        spawn.Type == SpawnType::Respawn ? "Respawn" : "Invisible";

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", typeStr);
    ImGui::Separator();
    ImGui::Text("%s", spawn.TagName.c_str());
    ImGui::Text("Position: %.2f, %.2f, %.2f",
        spawn.Position[0], spawn.Position[1], spawn.Position[2]);
    ImGui::Text("Forward:  %.2f, %.2f, %.2f",
        spawn.Forward[0], spawn.Forward[1], spawn.Forward[2]);
    ImGui::Text("Team:     %s", EnumToString::TeamToString(spawn.Team));
}

void UI_Navigation::DrawSelectionTeleporter(const ActiveTeleporter& tele) const
{
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "Teleporter");
    ImGui::Separator();
    ImGui::Text("%s", tele.TagName.c_str());
    ImGui::Text("Position: %.2f, %.2f, %.2f",
        tele.Position[0], tele.Position[1], tele.Position[2]);
    ImGui::Text("Forward:  %.2f, %.2f, %.2f",
        tele.Forward[0], tele.Forward[1], tele.Forward[2]);
    ImGui::Text("Up:       %.2f, %.2f, %.2f",
        tele.Up[0], tele.Up[1], tele.Up[2]);
    ImGui::Text("Channel:  %d", static_cast<int>(tele.Channel));
    ImGui::Text("Type:     %s", EnumToString::TeleporterTypeToString(tele.Type));

    ImGui::Spacing();
    ImGui::TextDisabled("Zone Shape:");
    switch (tele.ZoneShape.ShapeType)
    {
    case ShapeType::None:
        ImGui::Text("  None");
        break;
    case ShapeType::Cylinder:
        ImGui::Text("  Cylinder");
        ImGui::Text("  Radius: %.2f", tele.ZoneShape.Radius);
        ImGui::Text("  Top:    %.2f", tele.ZoneShape.Top);
        ImGui::Text("  Bottom: %.2f", tele.ZoneShape.Bottom);
        break;
    case ShapeType::Box:
        ImGui::Text("  Box");
        ImGui::Text("  Width:  %.2f", tele.ZoneShape.Radius);
        ImGui::Text("  Length: %.2f", tele.ZoneShape.Length);
        ImGui::Text("  Top:    %.2f", tele.ZoneShape.Top);
        ImGui::Text("  Bottom: %.2f", tele.ZoneShape.Bottom);
        break;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Allowed Objects:");
    this->DrawTeleporterAllowedObjects(tele.AllowedObjects);
}

void UI_Navigation::DrawTeleporterAllowedObjects(AllowedObjects allowed) const
{
    uint8_t raw = static_cast<uint8_t>(allowed);

    struct Flag { uint8_t bit; const char* label; bool inverted; };
    constexpr Flag flags[] = {
        { 0x01, "Players",         true  },
        { 0x02, "Ground Vehicles", false },
        { 0x04, "Heavy Vehicles",  false },
        { 0x08, "Flying Vehicles", false },
        { 0x10, "Projectiles",     false },
    };

    for (const auto& f : flags)
    {
        bool bitSet = (raw & f.bit) != 0;
        bool active = f.inverted ? !bitSet : bitSet;
        ImGui::TextColored(
            active ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f)
            : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
            "%s %s", active ? "[+]" : "[-]", f.label);
    }
}

void UI_Navigation::DrawSelectionLift(const ActiveLift& lift) const
{
    ImGui::TextColored(ImVec4(0.6f, 1.0f, 1.0f, 1.0f), "Lift");
    ImGui::Separator();
    ImGui::Text("%s", lift.TagName.c_str());
    ImGui::Text("Position:         %.2f, %.2f, %.2f",
        lift.Position[0], lift.Position[1], lift.Position[2]);
    ImGui::Text("Forward:          %.2f, %.2f, %.2f",
        lift.Forward[0], lift.Forward[1], lift.Forward[2]);
    ImGui::Text("Up:               %.2f, %.2f, %.2f",
        lift.Up[0], lift.Up[1], lift.Up[2]);
    ImGui::Text("Angle Type:       %s",
        EnumToString::AngleTypeToString(lift.AngleType));
    ImGui::Text("Force Type:       %s",
        EnumToString::ForceTypeToString(lift.ForceType));
    ImGui::Text("Launch Direction: %.2f, %.2f, %.2f",
        lift.LaunchDirection[0], lift.LaunchDirection[1], lift.LaunchDirection[2]);
}

void UI_Navigation::DrawSelectionShield(const ActiveShield& shield) const
{
    const char* typeStr =
        shield.Type == ShieldType::OneWay ? "One-Way" :
        shield.Type == ShieldType::TwoWay ? "Two-Way" : "Blocker";

    ImGui::TextColored(ImVec4(0.85f, 0.4f, 1.0f, 1.0f), "Shield");
    ImGui::Separator();
    ImGui::Text("%s", shield.TagName.c_str());
    ImGui::Text("Position: %.2f, %.2f, %.2f",
        shield.Position[0], shield.Position[1], shield.Position[2]);
    ImGui::Text("Forward:  %.2f, %.2f, %.2f",
        shield.Forward[0], shield.Forward[1], shield.Forward[2]);
    ImGui::Text("Up:       %.2f, %.2f, %.2f",
        shield.Up[0], shield.Up[1], shield.Up[2]);
    ImGui::Text("Type:     %s", typeStr);

    if (shield.BlockDirection.has_value())
    {
        ImGui::Text("Block Direction: %.2f, %.2f, %.2f",
            (*shield.BlockDirection)[0],
            (*shield.BlockDirection)[1],
            (*shield.BlockDirection)[2]);
    }
}

void UI_Navigation::DrawSelectionObjectiveSpawn(
    const ActiveObjectiveSpawn& spawn) const
{
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Objective Spawn");
    ImGui::Separator();
    ImGui::Text("%s", spawn.TagName.c_str());
    ImGui::Text("Position: %.2f, %.2f, %.2f",
        spawn.Position[0], spawn.Position[1], spawn.Position[2]);
    ImGui::Text("Forward:  %.2f, %.2f, %.2f",
        spawn.Forward[0], spawn.Forward[1], spawn.Forward[2]);
    ImGui::Text("Up:       %.2f, %.2f, %.2f",
        spawn.Up[0], spawn.Up[1], spawn.Up[2]);
    ImGui::Text("Team:     %s", EnumToString::TeamToString(spawn.Team));

    ImGui::Spacing();
    ImGui::TextDisabled("Zone Shape:");
    switch (spawn.ZoneShape.ShapeType)
    {
    case ShapeType::None:
        ImGui::Text("  None");
        break;
    case ShapeType::Cylinder:
        ImGui::Text("  Cylinder");
        ImGui::Text("  Radius: %.2f", spawn.ZoneShape.Radius);
        ImGui::Text("  Top:    %.2f", spawn.ZoneShape.Top);
        ImGui::Text("  Bottom: %.2f", spawn.ZoneShape.Bottom);
        break;
    case ShapeType::Box:
        ImGui::Text("  Box");
        ImGui::Text("  Width:  %.2f", spawn.ZoneShape.Radius);
        ImGui::Text("  Length: %.2f", spawn.ZoneShape.Length);
        ImGui::Text("  Top:    %.2f", spawn.ZoneShape.Top);
        ImGui::Text("  Bottom: %.2f", spawn.ZoneShape.Bottom);
        break;
    }
}

void UI_Navigation::DrawSelectionObjective(const ActiveObjective& obj) const
{
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
        "%s", obj.IsEquipped ? "Objective (Carried)" : "Objective");
    ImGui::Separator();
    ImGui::Text("%s", obj.TagName.c_str());
    ImGui::Text("Position:         %.2f, %.2f, %.2f",
        obj.Position[0], obj.Position[1], obj.Position[2]);
    ImGui::Text("Forward:          %.2f, %.2f, %.2f",
        obj.Forward[0], obj.Forward[1], obj.Forward[2]);
    ImGui::Text("Up:               %.2f, %.2f, %.2f",
        obj.Up[0], obj.Up[1], obj.Up[2]);
    ImGui::Text("Linear Velocity:  %.2f, %.2f, %.2f",
        obj.LinearVelocity[0], obj.LinearVelocity[1], obj.LinearVelocity[2]);
    ImGui::Text("Angular Velocity: %.2f, %.2f, %.2f",
        obj.AngularVelocity[0], obj.AngularVelocity[1], obj.AngularVelocity[2]);
    ImGui::Text("Team:             %s", EnumToString::TeamToString(obj.Team));
    ImGui::Text("Is Equipped:      %s", obj.IsEquipped ? "Yes" : "No");

    if (obj.IsEquipped)
        ImGui::Text("Carrier Handle:   0x%08X", obj.CarrierHandle);
}

void UI_Navigation::DrawSelectionDestructible(const ActiveDestructible& dest) const
{
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Destructible");
    ImGui::Separator();
    ImGui::Text("%s", dest.TagName.c_str());
    ImGui::Text("Position:         %.2f, %.2f, %.2f",
        dest.Position[0], dest.Position[1], dest.Position[2]);
    ImGui::Text("Forward:          %.2f, %.2f, %.2f",
        dest.Forward[0], dest.Forward[1], dest.Forward[2]);
    ImGui::Text("Up:               %.2f, %.2f, %.2f",
        dest.Up[0], dest.Up[1], dest.Up[2]);
    ImGui::Text("Linear Velocity:  %.2f, %.2f, %.2f",
        dest.LinearVelocity[0], dest.LinearVelocity[1], dest.LinearVelocity[2]);
    ImGui::Text("Angular Velocity: %.2f, %.2f, %.2f",
        dest.AngularVelocity[0], dest.AngularVelocity[1], dest.AngularVelocity[2]);
    ImGui::Text("Type:             %s",
        EnumToString::DestructibleTypeToString(dest.Type));
    ImGui::Text("Health:           %.2f", dest.Health);
}