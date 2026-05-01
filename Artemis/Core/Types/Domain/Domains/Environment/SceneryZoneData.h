#pragma once

#include <string>
#include <cstdint>

// Subset of the MP object type enum relevant to scenery zones
// Full enum has 28 values; these are the ones observed in filtered data
enum class ScenMpZoneType : uint8_t
{
    Powerup = 4,
    PlayerSpawnLocation = 15,
    // Types 16,19,25,26,27 observed but unconfirmed names, stored as raw
};

// Shape of the boundary volume
enum class ScenBoundaryShape : uint8_t
{
    Unused = 0,
    Sphere = 1,
    Cylinder = 2,
    Box = 3,
};

struct SceneryZoneData
{
    std::string TagName;

    uint8_t MpZoneTypeRaw;   // raw value, use ScenMpZoneType when known
    ScenBoundaryShape BoundaryShape;

    // Cylinder: WidthRadius = radius, BoxLength unused
    // Box:      WidthRadius = half-width X, BoxLength = half-width Y
    float BoundaryWidthRadius;
    float BoundaryBoxLength;
    float BoundaryHeight;

    // bit 2 = ValidInitialPlayerSpawn
    bool IsValidInitialSpawn;
};