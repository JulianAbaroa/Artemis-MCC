#pragma once

#include "Core/Types/Sources/Tables/Object/Specific/CrateObject.h"

#include <vector>
#include <array>

// --- Obstacles (bloc & scen) ---

struct Obstacle
{
    uint32_t Handle;
    std::string TagName;
    std::array<float, 3> Position;
    std::array<float, 3> Forward;
    std::array<float, 3> Up;
    std::array<float, 3> LinearVelocity;
    std::array<float, 3> AngularVelocity;
    float BoundingRadius;
};

// --- Spawns (scen) ---

enum class SpawnType : uint8_t
{
    Initial,
    Respawn,
    Invisible,
};

struct Spawn
{
    uint32_t Handle;
    std::string TagName;
    std::array<float, 3> Position;
    std::array<float, 3> Forward;
    SpawnType Type;
    Team Team;
};

// --- Teleports (bloc) ---

enum class TeleporterType : uint8_t
{
    Sender,
    Receiver,
    TwoWay,
};

struct Teleporter
{
    uint32_t Handle;
    std::string TagName;
    std::array<float, 3> Position;
    std::array<float, 3> Forward;
    std::array<float, 3> Up;
    TeleporterType Type;
    ZoneShape ZoneShape;
    AllowedObjects AllowedObjects;
    uint8_t Channel;

    std::vector<std::array<float, 3>> DestinationPositions;
};

// --- Lifts (bloc) ---

struct Lift
{
    uint32_t Handle;
    std::string TagName;

    std::array<float, 3> Position;
    std::array<float, 3> Forward;
    std::array<float, 3> Up;

    AngleType AngleType;
    ForceType ForceType;

    std::array<float, 3> LaunchDirection;
};

// --- Shields (bloc) ---

struct Shield
{
    uint32_t Handle;
    std::string TagName;
    std::array<float, 3> Position;
    std::array<float, 3> Forward;
    std::array<float, 3> Up;
    ShieldType Type;

    std::optional<std::array<float, 3>> BlockDirection;
};

// --- Objective Spawns (bloc) ---

struct ObjectiveSpawn
{
    uint32_t Handle;
    std::string TagName;
    std::array<float, 3> Position;
    std::array<float, 3> Forward;
    std::array<float, 3> Up;
    ZoneShape ZoneShape;
    Team Team;
};

// --- Objectives (weap) ---

struct Objective
{
    uint32_t Handle;
    std::string TagName;
    uint32_t CarrierHandle;
    std::array<float, 3> Position;
    std::array<float, 3> Forward;
    std::array<float, 3> Up;
    std::array<float, 3> LinearVelocity;
    std::array<float, 3> AngularVelocity;
    bool IsEquipped;
    Team Team;
};

// --- Destructibles (bloc & eqip) ---

enum class DestructibleType : uint8_t
{ 
    Pallet,
    Explosive,
    PortableShield
};

struct Destructible
{
    uint32_t Handle;
    std::array<float, 3> Position;
    std::array<float, 3> Forward;
    std::array<float, 3> Up;
    std::array<float, 3> LinearVelocity;
    std::array<float, 3> AngularVelocity;
    DestructibleType Type;
    float Health;
    std::string TagName;
};

struct Fixtures
{
    std::vector<Obstacle> Obstacles;
    std::vector<Spawn> Spawns;
    std::vector<Teleporter> Teleporters;
    std::vector<Lift> Lifts;
    std::vector<Shield> Shields;
    std::vector<ObjectiveSpawn> ObjectiveSpawns;
    std::vector<Objective> Objectives;
    std::vector<Destructible> Destructibles;
};