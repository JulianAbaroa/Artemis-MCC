#pragma once

// --- Types ---

// Crate Object.
#include "Core/Types/Domain/Object/Specific/CrateObject.h"

// Classified Object.
#include "Core/Types/Domain/Classification/ClassifiedObject.h"

// ImGui.
#include "External/imgui/imgui.h"

#include <optional>
#include <cstdint>
#include <vector>
#include <string>
#include <array>

// ----- Static Data -----

struct BlocResult
{
    bool addedObstacle = false;
    bool addedTeleporter = false;
};

// ----- Dynamic Data -----

// --- SBSP ---

// Represents a reachable cluster from the AI's current cluster.
struct AINavigationLink
{
    // Index in the SbspGeometry this link points to.
    int32_t ClusterIndex;       

    // Which SbspGeometry. (-1 = same BSP)
    int32_t SbspIndex;    

    // World-space position of the portal/seam.
    std::array<float, 3> Centroid;  

    // BoundingRadius of the portal. (how wide the opening is)
    float PassRadius;             
};

// Snapshot of a cluster relevant to AI navigation.
struct AINavigationCluster
{
    int32_t ClusterIndex;
    int32_t SbspIndex;

    std::array<float, 3> Center;
    std::array<float, 3> BoundsMin;
    std::array<float, 3> BoundsMax;

    // All reachable neighbors. (intra + cross-BSP)
    std::vector<AINavigationLink> Links; 
};

// The full navigation snapshot pushed to State_Navigation every ~16ms.
struct AINavigationState
{
    // Where the AI is.
    int32_t SelfClusterIndex = -1;
    int32_t SelfSbspIndex = -1;

    // The full cluster graph.
    std::vector<AINavigationCluster> Clusters;
};

// --- Obstacles (bloc & scen) ---

// Represents an obstacle on the navigation for the AI.
struct ActiveObstacle
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

// Represents a spawn on the navigation to the AI.
struct ActiveSpawn
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

// Represents a teleport on the navigation to the AI.
struct ActiveTeleporter
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

// Represents a lift on the navigation to the AI.
struct ActiveLift
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

// Represents a shield on the navigation to the AI.
struct ActiveShield
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

// Represents a objective spawn on the navigation to the AI.
struct ActiveObjectiveSpawn
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

// Represents a objective on the navigation to the AI.
struct ActiveObjective
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

struct ActiveDestructible
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