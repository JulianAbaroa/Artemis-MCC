#pragma once

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <array>

struct LiveObject;
struct LiveInteraction;
struct ObjectNode;
struct PlayerTree;
struct Affordance;
struct VehicleObject;
struct SeatStatus;
enum class AffordanceBehavior : uint8_t;
enum class AffordanceActivation : uint8_t;
enum class ObjectRole : uint8_t;

using ObjectGraph = std::unordered_map<uint32_t, ObjectNode>;
using PlayerGraph = std::vector<PlayerTree>;
using ObjectTable = std::unordered_map<uint32_t, LiveObject>;

class State_ObjectTable;
class State_InteractionTable;
class State_Self;
class State_Classifier;
class State_ObjectGraph;
class State_PlayerGraph;
class State_Affordances;
class System_Logs;

struct Sys_Affordances_Deps
{
    State_ObjectTable& State_ObjectTable;
    State_InteractionTable& State_InteractionTable;
    State_Self& State_Self;
    State_Classifier& State_Classifier;
    State_ObjectGraph& State_ObjectGraph;
    State_PlayerGraph& State_PlayerGraph;
    State_Affordances& State_Affordances;
    System_Logs& System_Logs;
};

class System_Affordances
{
public:
    System_Affordances(Sys_Affordances_Deps deps) : m_Deps(deps) {}
    ~System_Affordances() = default;

    void Update();

    void Cleanup();

private:
    Sys_Affordances_Deps m_Deps;

    bool BuildVehicleAffordance(const LiveObject& object,
        const std::array<float, 3>& selfPosition,
        uint32_t selfBipedHandle, const LiveInteraction& interaction,
        const ObjectGraph& objectGraph, const PlayerGraph& playerGraph,
        const ObjectTable& objectTable, Affordance& out) const;

    Affordance BuildPickupAffordance(const LiveObject& object,
        ObjectRole role, const std::array<float, 3>& selfPosition,
        const LiveInteraction& interaction) const;

    Affordance BuildInteractionAffordance(const LiveObject& object,
        const std::array<float, 3>& selfPosition,
        const LiveInteraction& interaction) const;

    Affordance BuildAmmoAffordance(const LiveObject& object,
        const std::array<float, 3>& selfPosition, ObjectRole role) const;

    bool ResolveSeatStatuses(const VehicleObject& vehi, 
        const LiveObject& object, const ObjectGraph& objectGraph,
        const ObjectTable& objectTable, const PlayerGraph& playerGraph,
        const std::array<float, 3>& selfPosition, uint32_t selfBipedHandle,
        std::vector<SeatStatus>& outSeats) const;

    std::vector<uint32_t> CollectVehiclePartHandles(uint32_t vehicleHandle,
        const ObjectGraph& objectGraph, const ObjectTable& objectTable) const;

    std::vector<uint32_t> CollectBipedChildHandles(uint32_t vehicleHandle,
        const ObjectGraph& objectGraph, const ObjectTable& objectTable) const;

    float Distance(const float a[3], const float b[3]) const;
    float Distance(const std::array<float, 3>& a, const std::array<float, 3>& b) const;

    std::vector<AffordanceBehavior> DeriveBehaviors(ObjectRole role) const;
    AffordanceActivation DeriveActivation(ObjectRole role) const;
};