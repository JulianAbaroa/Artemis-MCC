#pragma once

#include "External/imgui/imgui.h"

#include <cstdint>

// ----- Canvas -----

struct MapTransform
{
    ImVec2 CanvasPos;
    ImVec2 CanvasSize;
    float  WorldMinX = 0.0f;
    float  WorldMinY = 0.0f;
    float  Scale = 1.0f;
    float  OffsetX = 0.0f;
    float  OffsetY = 0.0f;
};

// ----- Layers -----

enum class MapLayer : uint32_t
{
    // Navigation.
    Clusters = 1 << 0,
    Obstacles = 1 << 1,
    Spawns = 1 << 2,
    Teleporters = 1 << 3,
    Lifts = 1 << 4,
    Shields = 1 << 5,
    ObjectiveSpawns = 1 << 6,
    Objectives = 1 << 7,
    Destructibles = 1 << 8,

    // Environment.
    PhysicsInstances = 1 << 9,
};

// ----- Selection -----

enum class MapSelectionType : uint8_t
{
    None,

    // Navigation.
    Cluster,
    Obstacle,
    Spawn,
    Teleporter,
    Lift,
    Shield,
    ObjectiveSpawn,
    Objective,
    Destructible,

    // Environment.
    PhysicsInstance,
};

struct MapSelection
{
    MapSelectionType Type = MapSelectionType::None;
    int32_t          Index = -1;
};

// ----- Hit testing -----

struct MapCandidate
{
    MapSelectionType Type;
    int32_t          Index;
    float            ScreenRadius;
};