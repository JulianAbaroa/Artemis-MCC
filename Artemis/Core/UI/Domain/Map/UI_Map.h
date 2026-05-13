#pragma once

// Map.
#include "Core/UI/Utils/MapTypes.h"
#include "Core/UI/Utils/MapHelpers.h"

// Panels.
#include "Core/UI/Domain/Navigation/UI_Navigation.h"
#include "Core/UI/Domain/Environment/UI_Environment.h"

// ImGui.
#include "External/imgui/imgui.h"
#include "External/imgui/imgui_internal.h"

#include <vector>

class UI_Map
{
public:
    void Draw();
    void Cleanup();

private:
    // --- Canvas state ---

    float    m_MapZoom = 1.0f;
    ImVec2   m_MapOffset = ImVec2(0.0f, 0.0f);
    MapTransform m_Transform{};

    // --- Selection state ---

    MapSelection m_Selection{};
    bool m_AutoSwitchTab = true;
    bool m_RequestTabSwitch = false;
    int m_ActiveTab = 0;

    // --- Layer visibility ---

    uint32_t m_VisibleLayers = ~0u;

    // --- Domain panels ---

    UI_Navigation m_Navigation;
    UI_Environment m_Environment;

    // --- Left panel ---

    void DrawLeftPanel();
    void DrawTabNavigation();
    void DrawTabEnvironment() const;

    // --- Right panel (canvas) ---

    void DrawRightPanel();
    void DrawMapControls();
    void DrawMapCanvas();
    void DrawDomainToggle(const char* label,
        std::initializer_list<MapLayer> layers);
    void HandleMapInput();
    void HandleMapSelection();

    void ComputeWorldBounds(float& outMinX, float& outMinY,
        float& outMaxX, float& outMaxY) const;

    // --- Layer helpers ---

    void DrawLayerToggle(const char* label, MapLayer layer);
    bool IsLayerVisible(MapLayer layer) const;

    int32_t CountSbsps() const;
};