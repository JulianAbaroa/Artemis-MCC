#pragma once

// Types.
#include "Core/Types/Domain/Environment/EnvironmentTypes.h"

// ImGui.
#include "External/imgui/imgui.h"

#include <vector>
#include <array>

struct MapTransform;
struct MapSelection;
struct MapCandidate;
struct WorldRigidBody;
struct WorldShape;

class State_Environment;
class System_Debug;

struct UI_Environment_Dependencies
{
    State_Environment& State_Environment;
    System_Debug& System_Debug;
};

class UI_Environment
{
public:
    UI_Environment(UI_Environment_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~UI_Environment() = default;

    void Draw(const MapTransform& transform, const MapSelection& selection) const;
    void DrawLayers(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection, uint32_t visibleLayers) const;
    void DrawSelectionPanel(const MapSelection& selection) const;

    void FetchState();
    void Cleanup();

    // Hit testing, called by UI_Map during selection.
    void CollectCandidates(const MapTransform& transform,
        std::vector<MapCandidate>& candidates, uint32_t visibleLayers) const;

    // World bounds, used by UI_Map to expand canvas extents if needed.
    void GetWorldBounds(float& outMinX, float& outMinY,
        float& outMaxX, float& outMaxY) const;

private:
    UI_Environment_Dependencies m_Deps;

    std::vector<ActivePhysicsInstance> m_CachedInstances;

    // --- Draw layers ---

    void DrawPhysicsInstances(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection) const;

    static void DrawWorldRigidBody(ImDrawList* draw, const MapTransform& transform,
        const WorldRigidBody& wrb, ImU32 color, ImU32 fillColor);

    static void DrawWorldShape(ImDrawList* draw, const MapTransform& transform,
        const WorldShape& ws, ImU32 color, ImU32 fillColor);

    static void DrawCollFallback(ImDrawList* draw, const MapTransform& transform,
        const ActivePhysicsInstance& instance, ImU32 color, ImU32 fillColor);

    // --- Selection panel ---

    void DrawSelectionPhysicsInstance(
        const ActivePhysicsInstance& instance) const;
};