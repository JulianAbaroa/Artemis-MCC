module;

#include <algorithm>
#include <cmath>

module Egocentric.Affordance.System;

import Egocentric.Self.Type;

namespace
{
    using Class = Tables::Object::Type::Class::Class;
    using BoneMatrix = Tables::Object::Type::BoneMatrix::BoneMatrix;
    using Seat = Tables::Object::Type::Vehicle::Seat::Seat;
    using Classified = Relations::Classifier::Type::Classified;
    using Classifieds = std::vector<Classified>;
    using Self = Egocentric::Self::Type::Self;
}

namespace Egocentric::Affordance::System
{
    auto AffordanceService::Update() -> void
    {
        auto selfPtr = m_SelfStore.Acquire();
        if (!selfPtr) return;
        const ::Self& self = *selfPtr;

        if (!self.IsAlive)
        {
            m_AffordanceStore.Publish({});
            return;
        }

        const Vec3& selfPosition = self.Position;
        const std::uint32_t selfBipedHandle = self.BipedHandle;

        auto interactionPtr = m_InteractionStore.Acquire();
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectGraphPtr = m_ObjectGraphStore.Acquire();
        auto playerGraphPtr = m_PlayerGraphStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();

        if (!interactionPtr || !classifiedsPtr || !objectGraphPtr ||
            !playerGraphPtr || !objectTablePtr) return;

        const AliveInteraction& interaction = *interactionPtr;
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

            const AliveObject& object = it->second;
            if (object.Address == 0) continue;

            Affordance interactable;

            switch (classified.Role)
            {
            case Role::Vehicle:
            {
                if (!this->BuildVehicleAffordance(
                    object, selfPosition, selfBipedHandle, interaction,
                    nodes, playerTrees, objectTable, interactable))
                {
                    continue;
                }
                break;
            }

            // TODO: Equipped objects/objects with parents has garbage positions.
            // Find a way to get the actual position, probably by looking up the
            // bone indexes and binding them to the position of srthe correct bone.
            //case ObjectRole::VehiclePart:
            case Role::WeaponPickup:
                //case ObjectRole::WeaponEquipped:
            case Role::ArmorAbilityPickup:
                //case ObjectRole::ArmorAbilityEquipped:
            case Role::ObjectivePickup:
                //case ObjectRole::ObjectiveEquipped:
            case Role::GrenadePickup:
            case Role::AmmoPickup:
            case Role::Powerup:
            case Role::HealthStation:
                interactable = this->BuildAffordance(
                    object, classified.Role, selfPosition, interaction);
                break;

            default:
                continue;
            }

            results.push_back(std::move(interactable));
        }

        m_AffordanceStore.Publish(std::move(results));
    }

    auto AffordanceService::BuildVehicleAffordance(const AliveObject& object, 
        const Vec3& selfPosition, std::uint32_t selfBipedHandle, 
        const AliveInteraction& interaction, const ObjectGraph& objectGraph, 
        const PlayerGraph& playerGraph, const ObjectTable& objectTable, Affordance& out) const -> bool
    {
        const Vehicle* vehiPtr = std::get_if<Vehicle>(&object.Specific);
        if (!vehiPtr) return false;

        const Vehicle& vehi = *vehiPtr;

        std::vector<SeatStatus> seats;
        if (!this->ResolveSeatStatuses(
            vehi, object, objectGraph, objectTable, playerGraph,
            selfPosition, selfBipedHandle, seats))
        {
            return false;
        }

        std::vector<std::uint32_t> partHandles = this->CollectVehiclePartHandles(
            object.Handle, objectGraph, objectTable);

        out.Handle = object.Handle;
        out.Position = object.Position;
        out.Role = Role::Vehicle;
        out.Behaviors = this->DeriveBehaviors(Role::Vehicle);
        out.Activation = this->DeriveActivation(Role::Vehicle);
        out.Seats = std::move(seats);
        out.ChildHandles = std::move(partHandles);
        out.IsEngineSelected = (interaction.TargetObjectHandle == object.Handle);
        out.DistanceToPlayer = this->Distance(object.Position, selfPosition);

        return true;
    }

    auto AffordanceService::BuildAffordance(const AliveObject& object, Role role,
        const Vec3& selfPosition, const AliveInteraction& interaction) const -> Affordance
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

    auto AffordanceService::ResolveSeatStatuses(const Vehicle& vehi, 
        const AliveObject& object, const ObjectGraph& objectGraph, 
        const ObjectTable& objectTable, const PlayerGraph& playerGraph, 
        const Vec3& selfPosition, std::uint32_t selfBipedHandle, 
        std::vector<SeatStatus>& outSeats) const -> bool
    {
        if (!vehi.SeatLayout) return false;

        const auto bonesPtr = m_BoneMatricesStore.Get(object.Handle);
        if (!bonesPtr) return false;

        const std::size_t count = vehi.SeatLayout->Seats.size();
        outSeats.reserve(count);

        const std::vector<std::uint32_t> bipedChildren =
            this->CollectBipedChildHandles(object.Handle, objectGraph, objectTable);

        const auto& markers = vehi.SeatLayout->SeatMarkers;

        bool anyFreeSeat = false;
        bool anyFreeHijack = false;

        for (std::size_t i = 0; i < count; ++i)
        {
            const Seat& seat = vehi.SeatLayout->Seats[i];
            const SeatMarker& marker = vehi.SeatLayout->SeatMarkers[i];

            SeatStatus status;
            status.SeatName = seat.Name;
            status.IsHijackerSlot = seat.IsHijackerSlot;
            status.IsOccupied = !vehi.IsSeatFree(seat);

            if (status.IsOccupied)
            {
                const std::int32_t normalSeatCount = static_cast<std::int32_t>(
                    std::count_if(vehi.SeatLayout->Seats.begin(),
                        vehi.SeatLayout->Seats.end(),
                        [](const Seat& s)
                        {
                            return std::string(s.Name).find("Hijacker")
                                == std::string::npos;
                        }));

                if (normalSeatCount == 1 && bipedChildren.size() == 1)
                {
                    status.OccupyingBipedHandle = bipedChildren[0];
                }
                else
                {
                    for (std::uint32_t bipedHandle : bipedChildren)
                    {
                        if (bipedHandle == selfBipedHandle)
                        {
                            status.OccupyingBipedHandle = bipedHandle;
                            break;
                        }
                    }
                }
            }

            if (!status.IsOccupied && !status.IsHijackerSlot) anyFreeSeat = true;
            if (!status.IsOccupied && status.IsHijackerSlot) anyFreeHijack = true;

            std::optional<Vec3> worldPos;
            if (i < markers.size())
            {
                worldPos = this->GetSeatWorldPosition(*bonesPtr, markers[i], object);
            }
            else
            {
                worldPos = Vec3{ object.Position.X, object.Position.Y, object.Position.Z };
            }

            if (worldPos)
            {
                status.SeatWorldPosition = { worldPos->X, worldPos->Y, worldPos->Z };
                status.DistanceToPlayer = this->Distance(status.SeatWorldPosition, selfPosition);
            }

            outSeats.push_back(std::move(status));
        }

        const auto nodeIt = objectGraph.find(object.Handle);
        if (nodeIt != objectGraph.end())
        {
            for (std::uint32_t childHandle : nodeIt->second.ChildrenHandles)
            {
                auto childObjIt = objectTable.find(childHandle);
                if (childObjIt == objectTable.end()) continue;
                if (childObjIt->second.Profile.Class != Class::Vehicle) continue;

                const AliveObject& partObj = childObjIt->second;
                const Vehicle* partVehi =
                    std::get_if<Vehicle>(&partObj.Specific);
                if (!partVehi || !partVehi->SeatLayout) continue;

                const std::vector<std::uint32_t> partBipeds =
                    this->CollectBipedChildHandles(childHandle, objectGraph, objectTable);

                for (const Seat& seatInfo : partVehi->SeatLayout->Seats)
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

    auto AffordanceService::CollectVehiclePartHandles(
        std::uint32_t vehicleHandle, const ObjectGraph& objectGraph,
        const ObjectTable& objectTable) const -> std::vector<std::uint32_t>
    {
        std::vector<std::uint32_t> parts;

        auto nodeIt = objectGraph.find(vehicleHandle);
        if (nodeIt == objectGraph.end()) return parts;

        for (std::uint32_t childHandle : nodeIt->second.ChildrenHandles)
        {
            auto objIt = objectTable.find(childHandle);
            if (objIt == objectTable.end()) continue;
            if (objIt->second.Profile.Class == Class::Vehicle)
                parts.push_back(childHandle);
        }

        return parts;
    }

    auto AffordanceService::CollectBipedChildHandles(std::uint32_t vehicleHandle, 
        const ObjectGraph& objectGraph, const ObjectTable& objectTable) const -> std::vector<std::uint32_t>
    {
        std::vector<std::uint32_t> bipeds;

        auto nodeIt = objectGraph.find(vehicleHandle);
        if (nodeIt == objectGraph.end()) return bipeds;

        for (std::uint32_t childHandle : nodeIt->second.ChildrenHandles)
        {
            auto objIt = objectTable.find(childHandle);
            if (objIt == objectTable.end()) continue;
            if (objIt->second.Profile.Class == Class::Biped)
                bipeds.push_back(childHandle);
        }

        return bipeds;
    }

    auto AffordanceService::GetSeatWorldPosition(const BoneMatrixTable& bones, 
        const SeatMarker& seatMarker, const AliveObject& vehicleObject) const -> std::optional<Vec3>
    {
        if (seatMarker.NodeIndex < 0)
        {
            return Vec3{ vehicleObject.Position.X,
                         vehicleObject.Position.Y,
                         vehicleObject.Position.Z };
        }

        if (static_cast<std::size_t>(seatMarker.NodeIndex) >= bones.Matrices.size())
            return std::nullopt;

        const BoneMatrix& bone = bones.Matrices[seatMarker.NodeIndex];

        if (!m_CollidableService.IsBoneMatrixValid(bone)) return std::nullopt;

        const auto& r = bone.Rotation;
        const Vec3& v = seatMarker.LocalTranslation;

        return Vec3{
            r[0] * v.X + r[3] * v.Y + r[6] * v.Z + bone.Translation[0],
            r[1] * v.X + r[4] * v.Y + r[7] * v.Z + bone.Translation[1],
            r[2] * v.X + r[5] * v.Y + r[8] * v.Z + bone.Translation[2]
        };
    }

    // --- Shared helpers ---

    auto AffordanceService::Distance(const float a[3], const float b[3]) const -> float
    {
        float dx = a[0] - b[0];
        float dy = a[1] - b[1];
        float dz = a[2] - b[2];
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    auto AffordanceService::Distance(const Vec3& a, const Vec3& b) const -> float
    {
        float dx = a.X - b.X;
        float dy = a.Y - b.Y;
        float dz = a.Z - b.Z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    auto AffordanceService::DeriveBehaviors(Role role) const -> std::vector<AffordanceBehavior>
    {
        std::vector<AffordanceBehavior> behaviors;
        switch (role)
        {
        case Role::AmmoPickup:
        case Role::WeaponPickup:
        case Role::ArmorAbilityPickup:
            behaviors.push_back(AffordanceBehavior::Pickup);
            break;
        case Role::Vehicle:
            behaviors.push_back(AffordanceBehavior::EnterVehicle);
            break;
        case Role::HealthStation:
            behaviors.push_back(AffordanceBehavior::Interact);
            break;
        default:
            break;
        }
        return behaviors;
    }

    auto AffordanceService::DeriveActivation(Role role) const -> AffordanceActivation
    {
        switch (role)
        {
        case Role::WeaponPickup:
        case Role::ArmorAbilityPickup:
        case Role::Vehicle:
        case Role::HealthStation:
            return AffordanceActivation::KeyPress;

        case Role::AmmoPickup:
            return AffordanceActivation::Proximity;
        default:
            return AffordanceActivation::None;
        }
    }

    auto AffordanceService::Cleanup() -> void
    {
        m_AffordanceStore.Cleanup();
        m_LogsService.Message("[AffordanceService] INFO: Cleanup completed.");
    }
}