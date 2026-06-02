#include "pch.h"

#include "UI_Map.h"

#include "Core/UI/Map/Navigation/UI_Navigation.h"
#include "Core/UI/Map/Environment/UI_Environment.h"

#include "Core/Systems/Logs/System_Logs.h"

#include <algorithm>
#include <limits>
#include <cmath>

void UI_Map::Draw()
{
    if (!m_Visible) return;
    
    this->FirstDraw();
    this->ApplyResetIfRequested("Map");

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (UI_Tab::s_Locked) flags |= ImGuiWindowFlags_NoMove;

    ImGui::Begin("Map", &m_Visible, flags);

    m_Deps.UI_Navigation.FetchState();
    m_Deps.UI_Environment.FetchState();

    if (m_Deps.UI_Navigation.GetGraph().empty())
    {
        ImGui::TextDisabled("No SBSP geometry loaded.");
        ImGui::End();
        return;
    }

    const float totalWidth = ImGui::GetContentRegionAvail().x;
    const float colWidth = totalWidth * 0.5f;

    ImGui::BeginChild("MapLeft", ImVec2(colWidth - 8.0f, 0), false);
    this->DrawLeftPanel();
    ImGui::EndChild();

    ImGui::SameLine();

    flags = ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginChild("MapRight", ImVec2(colWidth, 0), false, flags);
    this->DrawRightPanel();
    ImGui::EndChild();

    ImGui::End();
}

void UI_Map::DrawLeftPanel()
{
    auto& graph = m_Deps.UI_Navigation.GetGraph();
    int graphSize = static_cast<int>(graph.size());

    ImGui::TextDisabled("Graph: %d clusters across %d SBSPs", 
        graphSize, this->CountSbsps());

    ImGui::Separator();

    if (!ImGui::BeginTabBar("##map_tabs")) return;

    ImGuiTabItemFlags navFlags = 
        (m_ActiveTab == 0 && m_RequestTabSwitch) ? 
        ImGuiTabItemFlags_SetSelected : 0;

    ImGuiTabItemFlags envFlags = 
        (m_ActiveTab == 1 && m_RequestTabSwitch) ? 
        ImGuiTabItemFlags_SetSelected : 0;

    m_RequestTabSwitch = false;

    if (ImGui::BeginTabItem("Navigation", nullptr, navFlags))
    {
        m_ActiveTab = 0;
        m_Deps.UI_Navigation.Draw(m_Transform, m_Selection);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Environment", nullptr, envFlags))
    {
        m_ActiveTab = 1;
        m_Deps.UI_Environment.Draw(m_Transform, m_Selection);
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

int32_t UI_Map::CountSbsps() const
{
    int32_t max = -1;

    for (const auto& navCluster : m_Deps.UI_Navigation.GetGraph())
    {
        if (navCluster.SbspIndex > max)
        {
            max = navCluster.SbspIndex;
        }
    }

    return max + 1;
}

void UI_Map::DrawRightPanel()
{
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Map View");
    ImGui::Separator();

    this->DrawMapControls();

    ImGui::Spacing();

    this->DrawMapCanvas();
}

void UI_Map::DrawMapControls()
{
    ImGui::Checkbox("Auto-switch", &m_AutoSwitchTab);

    ImGui::SameLine();

    const float navWidth = ImGui::CalcTextSize("Navigation").x + 60.0f;
    ImGui::SetNextItemWidth(navWidth);

    if (ImGui::BeginCombo("##nav_layers", "Navigation"))
    {
        this->DrawDomainToggle("Navigation##domain", {
            MapLayer::Clusters, MapLayer::Obstacles,
            MapLayer::Spawns,MapLayer::Teleporters,
            MapLayer::Lifts, MapLayer::Shields,
            MapLayer::ObjectiveSpawns, MapLayer::Objectives,
            MapLayer::Destructibles, MapLayer::KillZones,
            MapLayer::SafeZones, MapLayer::RenderMesh,
        });

        ImGui::Separator();

        this->DrawLayerToggle("Clusters", MapLayer::Clusters);
        this->DrawLayerToggle("Obstacles", MapLayer::Obstacles);
        this->DrawLayerToggle("Spawns", MapLayer::Spawns);
        this->DrawLayerToggle("Teleporters", MapLayer::Teleporters);
        this->DrawLayerToggle("Lifts", MapLayer::Lifts);
        this->DrawLayerToggle("Shields", MapLayer::Shields);
        this->DrawLayerToggle("Objective Spawns", MapLayer::ObjectiveSpawns);
        this->DrawLayerToggle("Objectives", MapLayer::Objectives);
        this->DrawLayerToggle("Destructibles", MapLayer::Destructibles);
        this->DrawLayerToggle("Kill Zones", MapLayer::KillZones);
        this->DrawLayerToggle("Safe Zones", MapLayer::SafeZones);
        this->DrawLayerToggle("Collision Mesh", MapLayer::RenderMesh);

        ImGui::EndCombo();
    }

    ImGui::SameLine();

    const float envWidth = ImGui::CalcTextSize("Environment").x + 60.0f;
    ImGui::SetNextItemWidth(envWidth);

    if (ImGui::BeginCombo("##env_layers", "Environment"))
    {
        this->DrawDomainToggle("Environment##domain", {
            MapLayer::PhysicsInstances,
        });

        ImGui::Separator();

        this->DrawLayerToggle("Physics Instances", MapLayer::PhysicsInstances);

        ImGui::EndCombo();
    }

    ImGui::SameLine();
}

void UI_Map::DrawDomainToggle(const char* label,
    std::initializer_list<MapLayer> layers)
{
    bool allOn = true;

    for (MapLayer layer : layers)
    {
        if (!this->IsLayerVisible(layer)) 
        { 
            allOn = false; 
            break; 
        }
    }

    if (ImGui::Checkbox(label, &allOn))
    {
        for (MapLayer layer : layers)
        {
            if (allOn)
            {
                m_VisibleLayers |= static_cast<uint32_t>(layer);
            }
            else
            {
                m_VisibleLayers &= ~static_cast<uint32_t>(layer);
            }
        }
    }
}

bool UI_Map::IsLayerVisible(MapLayer layer) const
{
    return (m_VisibleLayers & static_cast<uint32_t>(layer)) != 0;
}

void UI_Map::DrawLayerToggle(const char* label, MapLayer layer)
{
    bool visible = this->IsLayerVisible(layer);
    if (ImGui::Checkbox(label, &visible))
    {
        if (visible)
            m_VisibleLayers |= static_cast<uint32_t>(layer);
        else
            m_VisibleLayers &= ~static_cast<uint32_t>(layer);
    }
}

void UI_Map::DrawMapCanvas()
{
    float worldMinX = (std::numeric_limits<float>::max)();
    float worldMinY = (std::numeric_limits<float>::max)();
    float worldMaxX = -(std::numeric_limits<float>::max)();
    float worldMaxY = -(std::numeric_limits<float>::max)();

    this->ComputeWorldBounds(worldMinX, worldMinY, worldMaxX, worldMaxY);

    const float worldW = worldMaxX - worldMinX;
    const float worldH = worldMaxY - worldMinY;
    if (worldW <= 0.0f || worldH <= 0.0f) return;

    const float  available = ImGui::GetContentRegionAvail().x;
    const float  side = available - 4.0f;

    const ImVec2 canvasSize = ImVec2(side, side);
    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("MapCanvas", canvasSize);

    m_Transform.CanvasPos = canvasPos;
    m_Transform.CanvasSize = canvasSize;
    m_Transform.WorldMinX = worldMinX;
    m_Transform.WorldMinY = worldMinY;

    float scaleX = canvasSize.x / worldW;
    float scaleY = canvasSize.y / worldH;
    float baseScale = (std::min)(scaleX, scaleY);

    m_Transform.Scale = baseScale * m_MapZoom;

    m_Transform.OffsetX = (canvasSize.x - worldW * baseScale) * 
        0.5f + m_MapOffset.x;

    m_Transform.OffsetY = (canvasSize.y - worldH * baseScale) * 
        0.5f + m_MapOffset.y;

    this->HandleMapInput();

    ImDrawList* draw = ImGui::GetWindowDrawList();

    draw->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, 
        canvasPos.y + canvasSize.y), IM_COL32(20, 20, 20, 255));

    draw->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, 
        canvasPos.y + canvasSize.y), IM_COL32(80, 80, 80, 255));

    draw->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, 
        canvasPos.y + canvasSize.y), true);

    // Navigation layers.
    m_Deps.UI_Navigation.DrawLayers(draw, m_Transform, m_Selection, 
        m_VisibleLayers);

    // Environment layers.
    m_Deps.UI_Environment.DrawLayers(draw, m_Transform, m_Selection, 
        m_VisibleLayers);

    draw->PopClipRect();

    this->HandleMapSelection();

    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        m_MapZoom = 1.0f;
        m_MapOffset = ImVec2(0.0f, 0.0f);
    }
}

void UI_Map::ComputeWorldBounds(float& outMinX, float& outMinY,
    float& outMaxX, float& outMaxY) const
{
    m_Deps.UI_Navigation.GetWorldBounds(outMinX, outMinY, outMaxX, outMaxY);
    m_Deps.UI_Environment.GetWorldBounds(outMinX, outMinY, outMaxX, outMaxY);
}

void UI_Map::HandleMapInput()
{
    if (!ImGui::IsItemHovered()) return;

    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f)
    {
        float newZoom = m_MapZoom * (wheel > 0.0f ? 1.1f : 0.9f);
        newZoom = (std::max)(0.5f, (std::min)(newZoom, 500.0f));

        ImVec2 mouse = ImGui::GetIO().MousePos;
        ImVec2 mouseRel = ImVec2(
            mouse.x - m_Transform.CanvasPos.x,
            mouse.y - m_Transform.CanvasPos.y);

        m_MapOffset.x = mouseRel.x - (mouseRel.x - m_MapOffset.x) * 
            (newZoom / m_MapZoom);

        m_MapOffset.y = mouseRel.y - (mouseRel.y - m_MapOffset.y) * 
            (newZoom / m_MapZoom);

        m_MapZoom = newZoom;
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        m_MapOffset.x += delta.x;
        m_MapOffset.y -= delta.y;
    }
}

void UI_Map::HandleMapSelection()
{
    if (!ImGui::IsItemHovered()) return;
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

    std::vector<MapCandidate> candidates;

    m_Deps.UI_Navigation.CollectCandidates(m_Transform, candidates, 
        m_VisibleLayers);

    m_Deps.UI_Environment.CollectCandidates(m_Transform, candidates, 
        m_VisibleLayers);

    if (candidates.empty())
    {
        m_Selection = {};
        return;
    }

    auto best = std::min_element(candidates.begin(), candidates.end(),
        [](const MapCandidate& a, const MapCandidate& b) {
            return a.ScreenRadius < b.ScreenRadius;
    });

    m_Selection = { best->Type, best->Index };

    if (m_AutoSwitchTab)
    {
        const bool isNavType =
            best->Type == MapSelectionType::Cluster ||
            best->Type == MapSelectionType::Obstacle ||
            best->Type == MapSelectionType::Spawn ||
            best->Type == MapSelectionType::Teleporter ||
            best->Type == MapSelectionType::Lift ||
            best->Type == MapSelectionType::Shield ||
            best->Type == MapSelectionType::ObjectiveSpawn ||
            best->Type == MapSelectionType::Objective ||
            best->Type == MapSelectionType::Destructible ||
            best->Type == MapSelectionType::KillZone ||
            best->Type == MapSelectionType::SafeZone;

        const bool isEnvType =
            best->Type == MapSelectionType::PhysicsInstance;

        if (isNavType) { m_ActiveTab = 0; m_RequestTabSwitch = true; }
        if (isEnvType) { m_ActiveTab = 1; m_RequestTabSwitch = true; }
    }
}

void UI_Map::Cleanup()
{
    m_MapZoom = 1.0f;
    m_MapOffset = ImVec2(0.0f, 0.0f);
    m_Selection = {};
    m_RequestTabSwitch = false;
    m_ActiveTab = 0;

    m_Deps.UI_Navigation.Cleanup();
    m_Deps.UI_Environment.Cleanup();
}