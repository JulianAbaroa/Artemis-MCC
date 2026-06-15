#pragma once

#include "Core/Types/Structure/Classified/Classified.h"

#include <vector>
#include <array>

enum class AffordanceBehavior : uint8_t
{
    None,
    Pickup,
    EnterVehicle,
    Avoid,
    Interact,
};

enum class AffordanceActivation : uint8_t
{
    None,
    KeyPress,   
    Proximity,
};

struct SeatStatus
{
    std::string SeatName;

    bool IsHijackerSlot = false;
    bool IsOccupied = false;

    uint32_t OccupyingBipedHandle = 0xFFFFFFFF;

    // TODO: resolve from ModeGeometry markers when available.
    std::array<float, 3> SeatWorldPosition{};

    float DistanceToPlayer = 0.0f;
};

struct Affordance
{
    uint32_t Handle = 0xFFFFFFFF;
    ObjectRole Role = ObjectRole::Unknown;

    std::vector<AffordanceBehavior> Behaviors;
    AffordanceActivation Activation = AffordanceActivation::None;

    float DistanceToPlayer = 0.0f;
    bool IsEngineSelected = false;

    std::array<float, 3> Position{};

    std::vector<SeatStatus> Seats;
    std::vector<uint32_t> ChildHandles;  
};