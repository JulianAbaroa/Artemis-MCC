#include "pch.h"

#include "System_Affordances.h"

#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"
#include "Core/States/Sources/Tables/Interaction/State_InteractionTable.h"
#include "Core/States/Structure/Classifier/State_Classifier.h"
#include "Core/States/Structure/Graph/Object/State_ObjectGraph.h"
#include "Core/States/Structure/Graph/Player/State_PlayerGraph.h"
#include "Core/States/Egocentric/Self/State_Self.h"
#include "Core/States/Egocentric/Affordances/State_Affordances.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include <algorithm>
#include <cmath>

void System_Affordances::Update()
{
    auto selfPtr = m_Deps.State_Self.Acquire();
    if (!selfPtr) return;
    const Self& self = *selfPtr;

    if (!self.IsAlive)
    {
        m_Deps.State_Affordances.Publish({});
        return;
    }

    const std::array<float, 3>& selfPosition = self.Position;
    const uint32_t selfBipedHandle = self.BipedHandle;

    auto interactionPtr = m_Deps.State_InteractionTable.Acquire();
    auto classifiedsPtr = m_Deps.State_Classifier.Acquire();
    auto objectGraphPtr = m_Deps.State_ObjectGraph.Acquire();
    auto playerGraphPtr = m_Deps.State_PlayerGraph.Acquire();
    auto objectTablePtr = m_Deps.State_ObjectTable.Acquire();

    if (!interactionPtr || !classifiedsPtr || !objectGraphPtr ||
        !playerGraphPtr || !objectTablePtr) return;

    const LiveInteraction& interaction = *interactionPtr;
    const Classifieds& classifieds = *classifiedsPtr;
    const ObjectGraph& nodes = *objectGraphPtr;
    const PlayerGraph& playerTrees = *playerGraphPtr;
    const ObjectTable& objectTable = *objectTablePtr;

    std::vector<Affordance> results;
    results.reserve(32);

    for (const Classified& classified : classifieds)
    {
        auto it = objectTable.find(classified.Handle);
        if (it == objectTable.end()) continue;

        const LiveObject& object = it->second;
        if (object.Address == 0) continue;

        Affordance interactable;

        switch (classified.Role)
        {
        case ObjectRole::Vehicle:
        {
            if (!this->BuildVehicleAffordance(
                object, selfPosition, selfBipedHandle, interaction,
                nodes, playerTrees, objectTable, interactable))
            {
                continue;
            }
            break;
        }

        case ObjectRole::WeaponPickup:
        case ObjectRole::ArmorAbilityPickup:
        case ObjectRole::ObjectivePickup:
            interactable = this->BuildPickupAffordance(
                object, classified.Role, selfPosition, interaction);
            break;

        case ObjectRole::HealthStation:
            interactable = this->BuildInteractionAffordance(
                object, selfPosition, interaction);
            break;

        case ObjectRole::AmmoPickup:
            interactable = this->BuildAmmoAffordance(
                object, selfPosition, classified.Role);
            break;

        default:
            continue;
        }

        results.push_back(std::move(interactable));
    }

    m_Deps.State_Affordances.Publish(std::move(results));
}

// --- BuildVehicleInteractable ---

bool System_Affordances::BuildVehicleAffordance(
    const LiveObject& object, const std::array<float, 3>& selfPosition,
    uint32_t selfBipedHandle, const LiveInteraction& interaction,
    const ObjectGraph& objectGraph, const PlayerGraph& playerGraph,
    const ObjectTable& objectTable, Affordance& out) const
{
    const VehicleObject* vehiPtr =
        std::get_if<VehicleObject>(&object.SpecificObject);
    if (!vehiPtr) return false;

    const VehicleObject& vehi = *vehiPtr;

    std::vector<SeatStatus> seats;
    if (!this->ResolveSeatStatuses(
        vehi, object, objectGraph, objectTable, playerGraph,
        selfPosition, selfBipedHandle, seats))
    {
        return false;
    }

    std::vector<uint32_t> partHandles = this->CollectVehiclePartHandles(
        object.Handle, objectGraph, objectTable);

    out.Handle = object.Handle;
    out.Position = object.Position;
    out.Role = ObjectRole::Vehicle;
    out.Behaviors = this->DeriveBehaviors(ObjectRole::Vehicle);
    out.Activation = this->DeriveActivation(ObjectRole::Vehicle);
    out.Seats = std::move(seats);
    out.ChildHandles = std::move(partHandles);
    out.IsEngineSelected = (interaction.TargetObjectHandle == object.Handle);
    out.DistanceToPlayer = this->Distance(object.Position, selfPosition);

    return true;
}

// --- BuildPickupInteractable ---

Affordance System_Affordances::BuildPickupAffordance(
    const LiveObject& object, ObjectRole role, 
    const std::array<float, 3>& selfPosition,
    const LiveInteraction& interaction) const
{
    Affordance out;
    out.Handle = object.Handle;
    out.Position = object.Position;
    out.Role = role;
    out.Behaviors = this->DeriveBehaviors(role);
    out.Activation = this->DeriveActivation(role);
    out.DistanceToPlayer = this->Distance(object.Position, selfPosition);
    out.IsEngineSelected = (interaction.TargetObjectHandle == object.Handle);
    return out;
}

// --- BuildInteractInteractable ---

Affordance System_Affordances::BuildInteractionAffordance(
    const LiveObject& object, const std::array<float, 3>& selfPosition,
    const LiveInteraction& interaction) const
{
    Affordance out;
    out.Handle = object.Handle;
    out.Position = object.Position;
    out.Role = ObjectRole::HealthStation;
    out.Behaviors = this->DeriveBehaviors(ObjectRole::HealthStation);
    out.Activation = this->DeriveActivation(ObjectRole::HealthStation);
    out.DistanceToPlayer = this->Distance(object.Position, selfPosition);
    out.IsEngineSelected = (interaction.TargetObjectHandle == object.Handle);
    return out;
}

Affordance System_Affordances::BuildAmmoAffordance(
    const LiveObject& object, const std::array<float, 3>& selfPosition,
    ObjectRole role) const
{
    Affordance out;
    out.Handle = object.Handle;
    out.Position = object.Position;
    out.Role = role;
    out.Behaviors = this->DeriveBehaviors(role);
    out.Activation = this->DeriveActivation(role);
    out.DistanceToPlayer = this->Distance(object.Position, selfPosition);
    return out;
}

// --- ResolveSeatStatuses ---

bool System_Affordances::ResolveSeatStatuses(
    const VehicleObject& vehi, const LiveObject& object,
    const ObjectGraph& objectGraph, const ObjectTable& objectTable,
    const PlayerGraph& playerGraph, const std::array<float, 3>& selfPosition,
    uint32_t selfBipedHandle, std::vector<SeatStatus>& outSeats) const
{
    if (!vehi.SeatLayout) return false;

    const std::vector<uint32_t> bipedChildren =
        this->CollectBipedChildHandles(object.Handle, objectGraph, objectTable);

    bool anyFreeSeat = false;
    bool anyFreeHijack = false;

    for (const SeatInfo& seatInfo : vehi.SeatLayout->seats)
    {
        SeatStatus seat;
        seat.SeatName = seatInfo.Name;
        seat.IsHijackerSlot = seatInfo.IsHijackerSlot;
        seat.IsOccupied = !vehi.IsSeatFree(seatInfo);

        seat.SeatWorldPosition = object.Position;
        seat.DistanceToPlayer =
            this->Distance(seat.SeatWorldPosition, selfPosition);

        if (seat.IsOccupied)
        {
            const int32_t normalSeatCount = static_cast<int32_t>(
                std::count_if(vehi.SeatLayout->seats.begin(),
                    vehi.SeatLayout->seats.end(),
                    [](const SeatInfo& s)
                    {
                        return std::string(s.Name).find("Hijacker")
                            == std::string::npos;
                    }));

            if (normalSeatCount == 1 && bipedChildren.size() == 1)
            {
                seat.OccupyingBipedHandle = bipedChildren[0];
            }
            else
            {
                for (uint32_t bipedHandle : bipedChildren)
                {
                    if (bipedHandle == selfBipedHandle)
                    {
                        seat.OccupyingBipedHandle = bipedHandle;
                        break;
                    }
                }
            }
        }

        if (!seat.IsOccupied && !seat.IsHijackerSlot) anyFreeSeat = true;
        if (!seat.IsOccupied && seat.IsHijackerSlot) anyFreeHijack = true;

        outSeats.push_back(std::move(seat));
    }

    const auto nodeIt = objectGraph.find(object.Handle);
    if (nodeIt != objectGraph.end())
    {
        for (uint32_t childHandle : nodeIt->second.ChildrenHandles)
        {
            auto childObjIt = objectTable.find(childHandle);
            if (childObjIt == objectTable.end()) continue;
            if (childObjIt->second.Profile.Class != ObjectClass::Vehicle) continue;

            const LiveObject& partObj = childObjIt->second;
            const VehicleObject* partVehi =
                std::get_if<VehicleObject>(&partObj.SpecificObject);
            if (!partVehi || !partVehi->SeatLayout) continue;

            const std::vector<uint32_t> partBipeds =
                this->CollectBipedChildHandles(childHandle, objectGraph, objectTable);

            for (const SeatInfo& seatInfo : partVehi->SeatLayout->seats)
            {
                SeatStatus seat;
                seat.SeatName = seatInfo.Name;
                seat.IsHijackerSlot = (std::string(seatInfo.Name).find("Hijacker")
                    != std::string::npos);
                seat.IsOccupied = !partVehi->IsSeatFree(seatInfo);

                seat.SeatWorldPosition = partObj.Position;
                seat.DistanceToPlayer =
                    this->Distance(seat.SeatWorldPosition, selfPosition);

                if (seat.IsOccupied && partBipeds.size() == 1)
                    seat.OccupyingBipedHandle = partBipeds[0];

                if (!seat.IsOccupied && !seat.IsHijackerSlot)
                    anyFreeSeat = true;

                outSeats.push_back(std::move(seat));
            }
        }
    }

    return anyFreeSeat || anyFreeHijack;
}

// --- CollectVehiclePartHandles ---

std::vector<uint32_t> System_Affordances::CollectVehiclePartHandles(
    uint32_t vehicleHandle, const ObjectGraph& objectGraph,
    const ObjectTable& objectTable) const
{
    std::vector<uint32_t> parts;

    auto nodeIt = objectGraph.find(vehicleHandle);
    if (nodeIt == objectGraph.end()) return parts;

    for (uint32_t childHandle : nodeIt->second.ChildrenHandles)
    {
        auto objIt = objectTable.find(childHandle);
        if (objIt == objectTable.end()) continue;
        if (objIt->second.Profile.Class == ObjectClass::Vehicle)
            parts.push_back(childHandle);
    }

    return parts;
}

// --- CollectBipedChildHandles ---

std::vector<uint32_t> System_Affordances::CollectBipedChildHandles(
    uint32_t vehicleHandle, const ObjectGraph& objectGraph,
    const ObjectTable& objectTable) const
{
    std::vector<uint32_t> bipeds;

    auto nodeIt = objectGraph.find(vehicleHandle);
    if (nodeIt == objectGraph.end()) return bipeds;

    for (uint32_t childHandle : nodeIt->second.ChildrenHandles)
    {
        auto objIt = objectTable.find(childHandle);
        if (objIt == objectTable.end()) continue;
        if (objIt->second.Profile.Class == ObjectClass::Biped)
            bipeds.push_back(childHandle);
    }

    return bipeds;
}

// --- Shared helpers ---

float System_Affordances::Distance(const float a[3], const float b[3]) const
{
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float System_Affordances::Distance(
    const std::array<float, 3>& a,
    const std::array<float, 3>& b) const
{
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::vector<AffordanceBehavior> System_Affordances::DeriveBehaviors(
    ObjectRole role) const
{
    std::vector<AffordanceBehavior> behaviors;
    switch (role)
    {
    case ObjectRole::AmmoPickup:
    case ObjectRole::WeaponPickup:
    case ObjectRole::ArmorAbilityPickup:
        behaviors.push_back(AffordanceBehavior::Pickup);
        break;
    case ObjectRole::Vehicle:
        behaviors.push_back(AffordanceBehavior::EnterVehicle);
        break;
    case ObjectRole::HealthStation:
        behaviors.push_back(AffordanceBehavior::Interact);
        break;
    default:
        break;
    }
    return behaviors;
}

AffordanceActivation System_Affordances::DeriveActivation(
    ObjectRole role) const
{
    switch (role)
    {
    case ObjectRole::WeaponPickup:
    case ObjectRole::ArmorAbilityPickup:
    case ObjectRole::Vehicle:
    case ObjectRole::HealthStation:
        return AffordanceActivation::KeyPress;

    case ObjectRole::AmmoPickup:
        return AffordanceActivation::Proximity;
    default:
        return AffordanceActivation::None;
    }
}

void System_Affordances::Cleanup()
{
    m_Deps.State_Affordances.Cleanup();
    m_Deps.System_Logs.Log("[Affordances] INFO: Cleanup completed.");
}