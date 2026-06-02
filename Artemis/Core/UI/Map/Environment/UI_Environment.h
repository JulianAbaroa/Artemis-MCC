#pragma once

#include "Core/Types/Environment/EnvironmentTypes.h"

#include "Core/UI/Launcher/UI_Tab.h"

#include "External/imgui/imgui.h"

#include <vector>
#include <array>

struct MapTransform;
struct MapSelection;
struct MapCandidate;
struct WorldRigidBody;
struct WorldShape;

// Dependencies.
class State_Environment;
class System_Logs;

struct UI_Environment_Dependencies
{
    State_Environment& State_Environment;
    System_Logs& System_Logs;
};

class UI_Environment : public UI_Tab
{
public:
    UI_Environment(UI_Environment_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~UI_Environment() = default;

    void Draw(const MapTransform& transform, const MapSelection& selection);
    void DrawLayers(ImDrawList* draw, const MapTransform& transform,
        const MapSelection& selection, uint32_t visibleLayers) const;
    void DrawSelectionPanel(const MapSelection& selection) const;

    void CollectCandidates(const MapTransform& transform,
        std::vector<MapCandidate>& candidates, uint32_t visibleLayers) const;

    void GetWorldBounds(float& outMinX, float& outMinY,
        float& outMaxX, float& outMaxY) const;

    void FetchState();
    void Cleanup();

private:
    UI_Environment_Dependencies m_Deps;

    std::vector<ActivePhysicsInstance> m_CachedInstances;

    // --- Selection panel ---

    void DrawSelectionPhysicsInstance(
        const ActivePhysicsInstance& instance) const;

    // --- Draw layers ---

    void DrawPhysicsInstances(ImDrawList* draw, 
        const MapTransform& transform, const MapSelection& selection) const;

    void DrawWorldRigidBody(ImDrawList* draw, 
        const MapTransform& transform, const WorldRigidBody& wrb, 
        ImU32 color, ImU32 fillColor) const;

    void DrawWorldShape(ImDrawList* draw, 
        const MapTransform& transform, const WorldShape& ws, 
        ImU32 color, ImU32 fillColor) const;

    void DrawCollFallback(ImDrawList* draw, 
        const MapTransform& transform, const ActivePhysicsInstance& instance, 
        ImU32 color, ImU32 fillColor) const;
};