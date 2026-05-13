#pragma once

// Types.
#include "Core/Types/Domain/Interactable/InteractableTypes.h"
#include "Core/Types/Domain/Classification/ClassifiedObject.h"
#include "Core/Types/Domain/Interaction/LiveInteraction.h"
#include "Core/Types/Domain/Object/LiveObject.h"
#include "Core/Types/Domain/Graph/ObjectNode.h"
#include "Core/Types/Domain/Graph/PlayerTree.h"

#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>

// Runs every ~16 ms. Reads from ObjectGraphState and PlayerTableState, 
// builds the list of AIInteractable objects for the requested player, 
// then pushes the result into InteractableState.
class System_Interactable
{ 
public:
    void BuildForMap();
    void UpdateInteractables();
    void Cleanup();

private:
    // --- Per-role builders ---

    bool BuildVehicleInteractable(const LiveObject& object,
        const LivePlayer& self, const LiveInteraction& interaction,
        const std::unordered_map<uint32_t, ObjectNode>& nodes,
        const std::vector<PlayerTree>& playerTrees,
        const std::unordered_map<uint32_t, LiveObject>& objectTable,
        AIInteractable& out) const;

    AIInteractable BuildPickupInteractable(const LiveObject& object,
        ObjectRole role, const LivePlayer& self,
        const LiveInteraction& interaction) const;

    AIInteractable BuildInteractInteractable(const LiveObject& object,
        const LivePlayer& self, const LiveInteraction& interaction) const;

    AIInteractable BuildAmmoInteractable(const LiveObject& object,
        const LivePlayer& self, ObjectRole role) const;

    // --- Vehicle helpers ---

    // Reads seat occupancy directly from VehicleObject memory offsets.
    // Returns false if VehicleObject is unavailable or no free seats exist.
    bool ResolveSeatStatuses(const VehicleObject& vehi, const LiveObject& object, 
        const std::unordered_map<uint32_t, ObjectNode>& nodes,
        const std::unordered_map<uint32_t, LiveObject>& objectTable,
        const std::vector<PlayerTree>& playerTrees,
        const LivePlayer& self, std::vector<SeatStatus>& outSeats) const;

    // Collects all VehiclePart child handles (e.g. FalconTurret, WarthogChaingun).
    std::vector<uint32_t> CollectVehiclePartHandles(uint32_t vehicleHandle,
        const std::unordered_map<uint32_t, ObjectNode>& nodes,
        const std::unordered_map<uint32_t, LiveObject>& objectTable) const;

    // Collects biped child handles of a vehicle (direct children only).
    std::vector<uint32_t> CollectBipedChildHandles(uint32_t vehicleHandle,
        const std::unordered_map<uint32_t, ObjectNode>& nodes,
        const std::unordered_map<uint32_t, LiveObject>& objectTable) const;

    // Tries to resolve which player owns a given biped handle.
    // Returns 0xFFFFFFFF if not found.
    uint32_t FindPlayerHandleForBiped(uint32_t bipedHandle,
        const std::vector<PlayerTree>& playerTrees) const;

    // --- Shared helpers ---

    float Distance(const float a[3], const float b[3]) const;
    float Distance(const std::array<float, 3>& a, const std::array<float, 3>& b) const;

    std::vector<InteractableBehavior> DeriveBehaviors(ObjectRole role) const;
    InteractableActivation DeriveActivation(ObjectRole role) const;
};