#pragma once

#include "Core/Types/Domain/Tables/InteractionTypes.h"
#include "Core/Types/Domain/Graph/GraphTypes.h"
#include "Core/Types/Domain/Domains/TagProfile.h"
#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>

// InteractableBehavior, behavior layer derived from TagGroup + TagProfile.
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
    // Human-readable seat name derived from AISeatType
    // (e.g. "Driver", "Gunner"). Falls back to "Seat" if vehi
    // tag data is unavailable.
    std::string SeatName;

    // Semantic type from the vehi tag's AI Seat Type enum.
    // AISeatType::None when vehi data is unavailable.
    //AISeatType AISeatType = AISeatType::None;

    bool IsOccupied = false;
    uint32_t OccupyingBipedHandle = 0xFFFFFFFF;

    // World-space position of this seat, computed each tick as:
    //   vehicle.Position + rotate(LocalPosition, vehicle.Forward, vehicle.Up)
    // Falls back to vehicle root if LocalPosition was not resolved from mode.
    float SeatWorldPosition[3] = {};

    // Distance from the AI player to this specific seat's world position.
    // Computed in PassesVehicleFilter each tick using self.WeaponPosition.
    // Use this (not interactable.DistanceToPlayer) for seat-level navigation.
    float DistanceToPlayer = 0.0f;

};


struct AIInteractable
{
    ObjectNode Node;

    std::vector<InteractableBehavior> Behaviors;
    InteractableActivation Activation = InteractableActivation::None;

    float DistanceToPlayer = 0.0f;

    // True if the engine currently has this object selected as the
    // interaction target for the self player.
    bool IsEngineSelected = false;

    // Occupancy is derived from the graph (bipeds are children of the vehicle node).
    // Seat position offsets will come from the vehicle tag when available;
    // For now, the root vehicle position is used as a fallback.
    std::vector<SeatStatus> Seats;

    // Handles of child interactables (e.g. seats of a vehicle).
    // Populated only when PrimaryGroup == Vehicle.
    std::vector<uint32_t> ChildHandles;

    // Convenience accessors.
    uint32_t Handle() const { return Node.Handle; }
    uint32_t ParentHandle() const { return Node.ParentHandle; }
    const std::string& TagName() const { return Node.TagName; }
    TagGroup PrimaryGroup() const { return Node.Profile.PrimaryGroup; }
    const float* Position() const { return Node.Position; }
    const float* Forward() const { return Node.Forward; }
    const float* Up() const { return Node.Up; }
};