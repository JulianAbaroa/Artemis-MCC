#pragma once

// Map.
#include "Core/UI/Utils/MapTypes.h"
#include "Core/UI/Utils/MapHelpers.h"

// ImGui.
#include "External/imgui/imgui.h"
#include "External/imgui/imgui_internal.h"

#include <vector>

class UI_Navigation;
class UI_Environment;

struct UI_Map_Dependencies
{
    UI_Navigation& UI_Navigation;
    UI_Environment& UI_Environment;
};

class UI_Map
{
public:
    UI_Map(UI_Map_Dependencies dependencies) : m_Deps(dependencies) {}
    ~UI_Map() = default;

    void Draw();
    void Cleanup();

private:
    UI_Map_Dependencies m_Deps;

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