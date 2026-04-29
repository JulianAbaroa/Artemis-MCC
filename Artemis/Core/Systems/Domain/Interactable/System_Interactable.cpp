#include "pch.h"
#include "Core/Common/Map/MapMagics.h"
#include "Core/Types/Domain/Domains/Environment/ModeGeometry.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Domain/Graph/State_ObjectGraph.h"
#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Map/Vehi/State_MapVehi.h"
#include "Core/States/Domain/Tables/State_PlayerTable.h"
#include "Core/States/Domain/Tables/State_InteractionTable.h"
#include "Core/States/Domain/Environment/State_Environment.h"
#include "Core/States/Domain/Interactable/State_Interactable.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Interactable/Vehi/System_VehiDataBuilder.h"
#include "Core/Systems/Domain/Interactable/System_Interactable.h"
#include "Core/Systems/Interface/System_Debug.h"
#include <algorithm>
#include <cmath>

void System_Interactable::BuildForMap()
{
	const int32_t tagCount =
		static_cast<int32_t>(g_pState->Domain->Map->GetTagsSize());

	int32_t builtCount = 0;

	for (int32_t i = 0; i < tagCount; ++i)
	{
		const Map_TagTableEntry& entry = g_pState->Domain->Map->GetTag(i);
		if (entry.TagGroupIndex < 0) continue;

		const uint32_t magic =
			g_pState->Domain->Map->GetGroupMagic(entry.TagGroupIndex);
		if (magic != MapMagics::k_VehiMagic) continue;

		const std::string tagName = g_pState->Domain->Map->GetTagName(i);
		if (tagName.empty()) continue;

		const VehiObject* vehi = g_pState->Domain->MapVehi->GetVehi(tagName);
		if (!vehi)
		{
			g_pSystem->Debug->Log("[InteractableSystem] WARNING:"
				" Vehi tag found in table but not loaded: ", tagName);
			continue;
		}

		VehicleData data =
			g_pSystem->Domain->VehiDataBuilder->BuildData(*vehi);

		g_pState->Domain->Interactable->AddVehiData(tagName, std::move(data));
		++builtCount;
	}

	g_pSystem->Debug->Log("[InteractableSystem] INFO:"
		" Interactable built. Vehi: %d", builtCount);
}

void System_Interactable::UpdateInteractables(uint32_t selfPlayerHandle)
{
	const LivePlayer* selfPtr =
		g_pState->Domain->PlayerTable->GetPlayer(selfPlayerHandle);
	if (!selfPtr) return;

	const LivePlayer self = *selfPtr;

	if (!self.IsAlive)
	{
		g_pState->Domain->Interactable->Cleanup();
		return;
	}

	const LiveInteraction interaction =
		g_pState->Domain->InteractionTable->GetLiveInteraction();

	const std::unordered_map<uint32_t, ObjectNode> nodes =
		g_pState->Domain->ObjectGraph->GetNodes();

	std::vector<AIInteractable> results;
	results.reserve(32);

	for (const auto& [handle, node] : nodes)
	{
		if (!this->PassesGroupFilter(node)) continue;

		std::vector<InteractableBehavior> behaviors = this->DeriveBehaviors(node.Profile);
		if (behaviors.empty()) continue;

		AIInteractable interactable;
		interactable.Node = node;
		interactable.Behaviors = behaviors;
		interactable.Activation = this->DeriveActivation(node.Profile.PrimaryGroup);

		this->SetPosition(node, nodes, interactable);

		interactable.DistanceToPlayer =
			this->Distance(interactable.Node.Position, self.WeaponPosition);

		// Child vehicles (turrets, sub-vehicles).
		if (node.Profile.PrimaryGroup == TagGroup::Vehicle)
		{
			uint32_t cursor = node.ChildHandle;
			while (cursor != 0xFFFFFFFF)
			{
				auto it = nodes.find(cursor);
				if (it == nodes.end()) break;
				if (it->second.Profile.PrimaryGroup == TagGroup::Vehicle)
					interactable.ChildHandles.push_back(cursor);
				cursor = it->second.NextSiblingHandle;
			}
		}

		if (!this->PassesContextFilter(node, behaviors, nodes, self.WeaponPosition, interactable))
			continue;

		if (handle == interaction.TargetObjectHandle &&
			interaction.TargetObjectHandle != 0xFFFFFFFF)
		{
			interactable.IsEngineSelected = true;
		}

		results.push_back(std::move(interactable));
	}

	g_pState->Domain->Interactable->SetInteractables(results);
}

void System_Interactable::Cleanup()
{
	g_pState->Domain->Interactable->Cleanup();
	g_pSystem->Debug->Log("[InteractableSystem] INFO: Cleanup completed.");
}

// --- Classification ---

bool System_Interactable::PassesGroupFilter(const ObjectNode& node) const
{
	switch (node.Profile.PrimaryGroup)
	{
	case TagGroup::Weapon:
	case TagGroup::Equipment:
	case TagGroup::Vehicle:
	case TagGroup::DeviceControl:
		return true;
	default:
		return false;
	}
}

std::vector<InteractableBehavior> System_Interactable::DeriveBehaviors(
	const TagProfile& profile) const
{
	std::vector<InteractableBehavior> behaviors;
	switch (profile.PrimaryGroup)
	{
	case TagGroup::Weapon:
	case TagGroup::Equipment:
		behaviors.push_back(InteractableBehavior::Pickup);
		break;
	case TagGroup::Vehicle:
		behaviors.push_back(InteractableBehavior::EnterVehicle);
		break;
	case TagGroup::DeviceControl:
		behaviors.push_back(InteractableBehavior::Interact);
		break;
	default:
		break;
	}
	return behaviors;
}

InteractableActivation System_Interactable::DeriveActivation(TagGroup group) const
{
	switch (group)
	{
	case TagGroup::Weapon:
	case TagGroup::Equipment:
	case TagGroup::Vehicle:
	case TagGroup::DeviceControl:
		return InteractableActivation::KeyPress;
	default:
		return InteractableActivation::None;
	}
}

// --- Context filters ---

bool System_Interactable::PassesContextFilter(
	const ObjectNode& node,
	const std::vector<InteractableBehavior>& behaviors,
	const std::unordered_map<uint32_t, ObjectNode>& nodes,
	const float playerPosition[3],
	AIInteractable& out) const
{
	for (InteractableBehavior behavior : behaviors)
	{
		switch (behavior)
		{
		case InteractableBehavior::Pickup:
		case InteractableBehavior::Interact:
			if (node.ParentHandle != 0xFFFFFFFF) return false;
			break;

		case InteractableBehavior::EnterVehicle:
			if (!this->PassesVehicleFilter(node, nodes, playerPosition, out))
				return false;
			break;

		default:
			break;
		}
	}
	return true;
}

bool System_Interactable::PassesVehicleFilter(
	const ObjectNode& node,
	const std::unordered_map<uint32_t, ObjectNode>& nodes,
	const float playerPosition[3],
	AIInteractable& out) const
{
	const std::vector<uint32_t> occupants = this->CollectDirectBipeds(node, nodes);

	// Resolve seats from ModeGeometry markers.
	this->ResolveSeats(node, occupants, playerPosition, nodes, out);

	// Vehicle is interactable if at least one seat is free.
	for (const auto& seat : out.Seats)
	{
		if (!seat.IsOccupied) return true;
	}

	// No ModeGeometry or no seats defined, fall back to occupant count heuristic.
	if (out.Seats.empty()) return occupants.empty();

	return false;
}

// --- Helpers ---

std::vector<uint32_t> System_Interactable::CollectDirectBipeds(
	const ObjectNode& vehicleNode,
	const std::unordered_map<uint32_t, ObjectNode>& nodes) const
{
	std::vector<uint32_t> bipeds;
	uint32_t cursor = vehicleNode.ChildHandle;
	while (cursor != 0xFFFFFFFF)
	{
		auto it = nodes.find(cursor);
		if (it == nodes.end()) break;
		if (it->second.Class == "bipd")
			bipeds.push_back(cursor);
		cursor = it->second.NextSiblingHandle;
	}
	return bipeds;
}

void System_Interactable::ResolveSeats(
	const ObjectNode& node,
	const std::vector<uint32_t>& occupants,
	const float playerPosition[3],
	const std::unordered_map<uint32_t, ObjectNode>& nodes,
	AIInteractable& out) const
{
	// Resolve source node for position and orientation.
	// Child objects have a buggy position, use the parent's position.
	const ObjectNode* origin = &node;
	if (node.ParentHandle != 0xFFFFFFFF)
	{
		auto it = nodes.find(node.ParentHandle);
		if (it != nodes.end()) origin = &it->second;
	}

	const VehicleData* vehicleData =
		g_pState->Domain->Interactable->GetVehiData(node.TagName);

	const int32_t occupantCount = static_cast<int32_t>(occupants.size());

	if (!vehicleData || vehicleData->Seats.empty())
	{
		SeatStatus seat;
		seat.SeatName = "Seat";
		seat.IsOccupied = !occupants.empty();
		seat.OccupyingBipedHandle = occupants.empty() ? 0xFFFFFFFF : occupants[0];
		std::memcpy(seat.SeatWorldPosition, origin->Position,
			sizeof(seat.SeatWorldPosition));
		seat.DistanceToPlayer = Distance(seat.SeatWorldPosition, playerPosition);
		out.Seats.push_back(seat);
		return;
	}

	const float fx = origin->Forward[0], fy = origin->Forward[1], fz = origin->Forward[2];
	const float ux = origin->Up[0], uy = origin->Up[1], uz = origin->Up[2];
	const float rx = fy * uz - fz * uy;
	const float ry = fz * ux - fx * uz;
	const float rz = fx * uy - fy * ux;

	const ModeGeometry* mode =
		g_pState->Domain->Environment->GetModeGeometry(node.TagName);

	int32_t seatIndex = 0;

	for (const auto& seatDef : vehicleData->Seats)
	{
		if (seatDef.InvalidForPlayer && seatDef.InvalidForNonPlayer) continue;
		if (seatDef.AISeatType == VehicleAISeatType::None) continue;
		if (seatDef.SeatMarkerNameId == 0) continue;

		if (mode)
		{
			bool markerExists = false;
			for (const auto& group : mode->MarkerGroups)
			{
				if (group.NameId == seatDef.SeatMarkerNameId)
				{
					markerExists = true; break;
				}
			}
			if (!markerExists) continue;
		}

		SeatStatus seat;

		switch (seatDef.AISeatType)
		{
		case VehicleAISeatType::Driver:     seat.SeatName = "Driver";     break;
		case VehicleAISeatType::Gunner:     seat.SeatName = "Gunner";     break;
		case VehicleAISeatType::Passenger:  seat.SeatName = "Passenger";  break;
		case VehicleAISeatType::SmallCargo: seat.SeatName = "SmallCargo"; break;
		case VehicleAISeatType::LargeCargo: seat.SeatName = "LargeCargo"; break;
		default:                            seat.SeatName = "Seat";       break;
		}

		seat.IsOccupied = (seatIndex < occupantCount);
		seat.OccupyingBipedHandle =
			seat.IsOccupied ? occupants[seatIndex] : 0xFFFFFFFF;

		bool markerResolved = false;

		if (mode && seatDef.SeatMarkerNameId != 0)
		{
			for (const auto& group : mode->MarkerGroups)
			{
				if (group.NameId != seatDef.SeatMarkerNameId) continue;
				if (group.Markers.empty()) break;

				const ModeMarker& marker = group.Markers[0];
				const float lx = marker.Translation.X;
				const float ly = marker.Translation.Y;
				const float lz = marker.Translation.Z;

				seat.SeatWorldPosition[0] = origin->Position[0] + lx * rx + ly * fx + lz * ux;
				seat.SeatWorldPosition[1] = origin->Position[1] + lx * ry + ly * fy + lz * uy;
				seat.SeatWorldPosition[2] = origin->Position[2] + lx * rz + ly * fz + lz * uz;

				markerResolved = true;
				break;
			}
		}

		if (!markerResolved)
		{
			seat.SeatWorldPosition[0] = origin->Position[0];
			seat.SeatWorldPosition[1] = origin->Position[1];
			seat.SeatWorldPosition[2] = origin->Position[2];
		}

		seat.DistanceToPlayer = Distance(seat.SeatWorldPosition, playerPosition);
		out.Seats.push_back(seat);
		++seatIndex;
	}
}

void System_Interactable::SetPosition(
	const ObjectNode& node,
	const std::unordered_map<uint32_t, ObjectNode>& nodes,
	AIInteractable& out) const
{
	// Child objects have a broken world position near zero in Reach MCC.
	// Use parent position when available.
	const ObjectNode* source = &node;

	if (node.ParentHandle != 0xFFFFFFFF)
	{
		auto it = nodes.find(node.ParentHandle);
		if (it != nodes.end())
			source = &it->second;
	}

	std::memcpy(out.Node.Position, source->Position, sizeof(out.Node.Position));
	std::memcpy(out.Node.Forward, source->Forward, sizeof(out.Node.Forward));
	std::memcpy(out.Node.Up, source->Up, sizeof(out.Node.Up));
}

float System_Interactable::Distance(const float a[3], const float b[3]) const
{
	float dx = a[0] - b[0];
	float dy = a[1] - b[1];
	float dz = a[2] - b[2];
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}