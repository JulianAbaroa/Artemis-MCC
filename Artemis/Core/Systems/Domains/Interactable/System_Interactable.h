#pragma once

#include <unordered_map>
#include <cstdint>
#include <string>
#include <vector>

struct LiveObject;
struct LivePlayer;
struct LiveInteraction;
struct ObjectNode;
struct PlayerTree;
struct AIInteractable;
struct VehicleObject;
struct SeatStatus;
enum class InteractableBehavior : uint8_t;
enum class InteractableActivation : uint8_t;
enum class ObjectRole : uint8_t;

class State_MapReader;
class State_MapVehi;
class State_MapEqip;
class State_MapWeap;
class State_MapProj;
class State_MapCtrl;
class State_ObjectTable;
class State_PlayerTable;
class State_InteractionTable;
class State_Classifier;
class State_ObjectGraph;
class State_PlayerGraph;
class State_Environment;
class State_Interactable;
class System_Classifier;
class System_ProjBuilder;
class System_VehiBuilder;
class System_WeapBuilder;
class System_Logs;

struct System_Interactable_Dependencies
{
    State_MapReader& State_Map;
    State_MapVehi& State_MapVehi;
    State_MapEqip& State_MapEqip;
    State_MapWeap& State_MapWeap;
    State_MapProj& State_MapProj;
    State_MapCtrl& State_MapCtrl;
    State_ObjectTable& State_ObjectTable;
    State_PlayerTable& State_PlayerTable;
    State_InteractionTable& State_InteractionTable;
    State_Classifier& State_Classification;
    State_ObjectGraph& State_ObjectGraph;
    State_PlayerGraph& State_PlayerGraph;
    State_Environment& State_Environment;
    State_Interactable& State_Interactable;
    System_Classifier& System_Classifier;
    System_ProjBuilder& System_ProjBuilder;
    System_VehiBuilder& System_VehiBuilder;
    System_WeapBuilder& System_WeapBuilder;
    System_Logs& System_Logs;
};

// Runs every ~16 ms. Reads from ObjectGraphState and PlayerTableState, 
// builds the list of AIInteractable objects for the requested player, 
// then pushes the result into InteractableState.
class System_Interactable
{ 
public:
    System_Interactable(System_Interactable_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~System_Interactable() = default;

    void BuildForMap();

    void UpdateInteractables();

    void Cleanup();

private:
    System_Interactable_Dependencies m_Deps;

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