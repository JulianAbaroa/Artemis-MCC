#pragma once

#include "Core/Common/Types/Domain/Interactable/InteractableTypes.h"
#include <unordered_map>
#include <cstdint>
#include <string>

struct LivePlayer;
struct ObjectNode;

// Runs every ~16 ms. Reads from ObjectGraphState and PlayerTableState, 
// builds the list of AIInteractable objects for the requested player, 
// then pushes the result into InteractableState.
class InteractableSystem
{ 
public:
	// Main entry point. Call once per tick from the game loop.
	// selfPlayerHandle — the Handle of the AI-controlled player.
	void UpdateInteractables(uint32_t selfPlayerHandle);
	void Cleanup();

private:
    // --- Classification ---

    // Filter by Tag Group, if Primary Group == Unknown, skip immediately.
    bool PassesGroupFilter(const ObjectNode& node) const;

    // Derive the behaviors corresponding to the TagGroup + TagProfile of the node.
    std::vector<InteractableBehavior> DeriveBehaviors(const TagProfile& profile) const;

    // Derive the activation corresponding to the TagGroup.
    InteractableActivation DeriveActivation(TagGroup group) const;

    // --- Context filters ---

    // Discard objects that do not pass runtime context conditions.
    bool PassesContextFilter(const ObjectNode& node,
        const std::vector<InteractableBehavior>& behaviors,
        const std::unordered_map<uint32_t, ObjectNode>& nodes,
        const float playerPosition[3],
        AIInteractable& out) const;

    // Filter and populate seats for vehicles.
    // Returns false if the vehicle has no free seats.
    bool PassesVehicleFilter(const ObjectNode& node,
        const std::unordered_map<uint32_t, ObjectNode>& nodes,
        const float playerPosition[3],
        AIInteractable& out) const;

    // --- Seat logic ---

    // Traverse the child chain of the vehicle node and collect direct bipeds.
    std::vector<uint32_t> CollectDirectBipeds(const ObjectNode& vehicleNode,
        const std::unordered_map<uint32_t, ObjectNode>& nodes) const;
};