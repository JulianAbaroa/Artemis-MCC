#pragma once


// Types.
#include "Core/Types/Domain/Navigation/NavigationTypes.h"

// Helpers.
#include "Core/UI/Utils/MapTypes.h"
#include "Core/UI/Utils/MapHelpers.h"
#include "Core/UI/Utils/Strings/EnumToString.h"
#include "Core/UI/Utils/Colors/TeamToColor.h"

// ImGui.
#include "External/imgui/imgui.h"

#include <vector>
#include <array>

class State_Navigation;

class UI_Navigation
{
public:
    UI_Navigation(State_Navigation& stateNavigation) : 
        m_State_Navigation(stateNavigation){}
    ~UI_Navigation() = default;

    void Draw(const MapTransform& transform, const MapSelection& selection);
    void DrawLayers(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection, uint32_t visibleLayers) const;
    void DrawSelectionPanel(const MapSelection& selection) const;

    void FetchState();
    void Cleanup();

    // Hit testing, called by UI_Map during selection.
    void CollectCandidates(const MapTransform& transform,
        std::vector<MapCandidate>& candidates, uint32_t visibleLayers) const;

    // World bounds, used by UI_Map to compute canvas extents.
    void GetWorldBounds(float& outMinX, float& outMinY,
        float& outMaxX, float& outMaxY) const;

    const std::vector<AINavigationCluster>& GetGraph() const { return m_CachedGraph; }

private:
    State_Navigation& m_State_Navigation;

    std::vector<AINavigationCluster> m_CachedGraph;
    std::vector<AINavigationCluster> m_CachedRawGraph;
    std::vector<ActiveObstacle>      m_CachedObstacles;
    std::vector<ActiveSpawn>         m_CachedSpawns;
    std::vector<ActiveTeleporter>    m_CachedTeleporters;
    std::vector<ActiveLift>          m_CachedLifts;
    std::vector<ActiveShield>        m_CachedShields;
    std::vector<ActiveObjectiveSpawn> m_CachedObjectiveSpawns;
    std::vector<ActiveObjective>     m_CachedObjectives;
    std::vector<ActiveDestructible>  m_CachedDestructibles;

    bool m_ShowAllClusters = false;

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
};