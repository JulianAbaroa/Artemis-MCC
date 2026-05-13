#pragma once

#include <string>
#include <cstdint>

enum class MachineType : uint16_t
{
    Door = 0,
    Platform = 1,
    Gear = 2,
};

enum class MachPathfindingPolicy : uint16_t
{
    Discs = 0,
    Sectors = 1,
    CutOut = 2,
    None = 3,
};

enum class MachCollisionResponse : uint16_t
{
    PauseUntilCrushed = 0,
    ReverseDirections = 1,
    Discs = 2,
};

struct MachineData
{
    std::string TagName;

    float BoundingRadius;

    // Machine type and navigation behaviour
    MachineType Type;
    MachPathfindingPolicy PathfindingPolicy;
    MachCollisionResponse CollisionResponse;

    // Flags
    bool IsElevator;
    bool PositionInterpolation;  // Flags_2 bit 2

    // Activation
    float AutomaticActivationRadius;
    float DoorOpenTime;

    // Movement timings, how long the machine takes to travel
    // Critical for AI: know when a platform/door will be traversable
    float PositionTransitionTime;
    float PositionAccelerationTime;
    float PowerAccelerationTime;
};