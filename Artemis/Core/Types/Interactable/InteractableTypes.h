#pragma once

#include "../Classified/Classified.h"
#include "../Object/ObjectProfile.h"
#include <cstdint>
#include <string>
#include <vector>
#include <array>

// Behavior layer derived from ObjectGroup + ObjectProfile.
enum class InteractableBehavior : uint8_t
{
    None,

    // Pick up object from the floor (weap/eqip without parent).
    Pickup,

    // Get into a vehicle with an available seat.
    EnterVehicle,

    // Avoid (projectiles, explosives).
    Avoid,

    // Activate mechanism (Reach has something like this?).
    Interact,
};

// How the player triggers the interaction.
enum class InteractableActivation : uint8_t
{
    // No direct interaction, world state entity.
    None,

    // Player must press the interact key while looking at the object.
    KeyPress,   

    // Triggered automatically when the player enters the object's hitbox.
    Proximity,
};

struct SeatStatus
{
    // Human-readable name from SeatInfo ("Driver", "Gunner", "Passenger", etc.)
    std::string SeatName;

    // True if this slot is a hijack position (not a rideable seat).
    bool IsHijackerSlot = false;

    // Read directly from VehicleObject memory offsets each tick.
    bool IsOccupied = false;

    // Known when: the occupant is a turret gunner (VehiclePart match)
    // or the self player. 0xFFFFFFFF otherwise.
    uint32_t OccupyingBipedHandle = 0xFFFFFFFF;

    // World position: vehicle root position as fallback.
    // TODO: resolve from ModeGeometry markers when available.
    std::array<float, 3> SeatWorldPosition{};

    float DistanceToPlayer = 0.0f;
};

struct AIInteractable
{
    uint32_t Handle = 0xFFFFFFFF;
    ObjectRole Role = ObjectRole::Unknown;

    std::vector<InteractableBehavior> Behaviors;
    InteractableActivation Activation = InteractableActivation::None;

    float DistanceToPlayer = 0.0f;
    bool IsEngineSelected = false;

    // Populated for Role == Vehicle only.
    std::vector<SeatStatus> Seats;

    // VehiclePart handles
    std::vector<uint32_t> ChildHandles;  
};