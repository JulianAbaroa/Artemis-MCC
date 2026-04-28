#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Graph/ObjectGraphState.h"
#include "Core/States/Domain/Map/Vehi/MapVehiState.h"
#include "Core/States/Domain/Tables/PlayerTableState.h"
#include "Core/States/Domain/Tables/InteractionTableState.h"
#include "Core/States/Domain/Interactable/InteractableState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/Interactable/InteractableSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <algorithm>
#include <cmath>

void InteractableSystem::UpdateInteractables(uint32_t selfPlayerHandle)
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

	// Engine-side interaction snapshot, tells us what the engine
	// currently considers interactable for the self player.
	const LiveInteraction interaction =
		g_pState->Domain->InteractionTable->GetLiveInteraction();

	const std::unordered_map<uint32_t, ObjectNode> nodes =
		g_pState->Domain->ObjectGraph->GetNodes();

	std::vector<AIInteractable> results;
	results.reserve(32);

	for (const auto& [handle, node] : nodes)
	{
		// Classification by TagGroup, O(1), without lookup by TagName.
		if (!this->PassesGroupFilter(node)) continue;

		// Derive behaviors from the Profile already built in ObjectGraphSystem.
		std::vector<InteractableBehavior> behaviors =
			this->DeriveBehaviors(node.Profile);

		if (behaviors.empty()) continue;

		AIInteractable interactable;
		interactable.Handle = handle;
		interactable.ParentHandle = node.ParentHandle;
		interactable.TagName = node.TagName;
		interactable.PrimaryGroup = node.Profile.PrimaryGroup;
		interactable.Profile = node.Profile;
		interactable.Behaviors = behaviors;
		interactable.Activation = this->DeriveActivation(node.Profile.PrimaryGroup);

		if (node.ParentHandle != 0xFFFFFFFF)
		{
			auto parentIt = nodes.find(node.ParentHandle);
			if (parentIt != nodes.end())
			{
				interactable.Position[0] = parentIt->second.Position[0];
				interactable.Position[1] = parentIt->second.Position[1];
				interactable.Position[2] = parentIt->second.Position[2];
				interactable.Forward[0] = parentIt->second.Forward[0];
				interactable.Forward[1] = parentIt->second.Forward[1];
				interactable.Forward[2] = parentIt->second.Forward[2];
				interactable.Up[0] = parentIt->second.Up[0];
				interactable.Up[1] = parentIt->second.Up[1];
				interactable.Up[2] = parentIt->second.Up[2];
			}
		}
		else
		{
			interactable.Position[0] = node.Position[0];
			interactable.Position[1] = node.Position[1];
			interactable.Position[2] = node.Position[2];
			interactable.Forward[0] = node.Forward[0];
			interactable.Forward[1] = node.Forward[1];
			interactable.Forward[2] = node.Forward[2];
			interactable.Up[0] = node.Up[0];
			interactable.Up[1] = node.Up[1];
			interactable.Up[2] = node.Up[2];
		}

		// Currently using the weapon position, this matters?
		float dx = interactable.Position[0] - self.WeaponPosition[0];
		float dy = interactable.Position[1] - self.WeaponPosition[1];
		float dz = interactable.Position[2] - self.WeaponPosition[2];

		float distanceToPlayer = std::sqrt(dx * dx + dy * dy + dz * dz);
		interactable.DistanceToPlayer = distanceToPlayer;

		// Populate child interactables for vehicles (turrets, sub-vehicles).
		if (node.Profile.PrimaryGroup == TagGroup::Vehicle)
		{
			uint32_t cursor = node.ChildHandle;
			while (cursor != 0xFFFFFFFF)
			{
				auto it = nodes.find(cursor);
				if (it == nodes.end()) break;

				if (it->second.Profile.PrimaryGroup == TagGroup::Vehicle)
				{
					interactable.ChildHandles.push_back(cursor);
				}

				cursor = it->second.NextSiblingHandle;
			}
		}

		// Runtime context filters (parent, seats, etc.).
		if (!this->PassesContextFilter(node, behaviors, nodes, self.WeaponPosition, interactable))
		{
			continue;
		}

		// Mark whichever object the engine has selected 
		// as the current interaction target.
		if (handle == interaction.TargetObjectHandle &&
			interaction.TargetObjectHandle != 0xFFFFFFFF)
		{
			interactable.IsEngineSelected = true;
		}
			
		results.push_back(std::move(interactable));
	}

	g_pState->Domain->Interactable->SetInteractables(results);
}

void InteractableSystem::Cleanup()
{
	g_pState->Domain->Interactable->Cleanup();
	g_pSystem->Debug->Log("[InteractableSystem] INFO: Cleanup completed.");
}

// Classification.

bool InteractableSystem::PassesGroupFilter(const ObjectNode& node) const
{
	return node.Profile.PrimaryGroup != TagGroup::Unknown;
}

std::vector<InteractableBehavior> InteractableSystem::DeriveBehaviors(
	const TagProfile& profile) const
{
	std::vector<InteractableBehavior> behaviors;

	switch (profile.PrimaryGroup)
	{
	case TagGroup::Weapon:
	case TagGroup::Equipment:
		// The parent filter is applied in PassesContextFilter.
		behaviors.push_back(InteractableBehavior::Pickup);
		break;

	case TagGroup::Vehicle:
		behaviors.push_back(InteractableBehavior::EnterVehicle);
		break;

	case TagGroup::DeviceControl:
		behaviors.push_back(InteractableBehavior::Interact);
		break;

	case TagGroup::Projectile:
		behaviors.push_back(InteractableBehavior::Avoid);
		break;

	case TagGroup::Crate:
	case TagGroup::Scenery:
		behaviors.push_back(InteractableBehavior::None);
		break;

	default:
		break;
	}

	return behaviors;
}

InteractableActivation InteractableSystem::DeriveActivation(TagGroup group) const
{
	switch (group)
	{
	case TagGroup::Weapon:
	case TagGroup::Equipment:
	case TagGroup::Vehicle:
	case TagGroup::DeviceControl:
		return InteractableActivation::KeyPress;

	case TagGroup::Scenery:
	case TagGroup::Crate:
	case TagGroup::Projectile:
		return InteractableActivation::None;

	default:
		return InteractableActivation::None;
	}
}

// Context filters.

bool InteractableSystem::PassesContextFilter(const ObjectNode& node,
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
			// weap/eqip/ctrl on the ground = no parent.
			if (node.ParentHandle != 0xFFFFFFFF) return false;
			break;

		case InteractableBehavior::EnterVehicle:
			// Vehicle must have at least one free seat.
			if (!this->PassesVehicleFilter(node, nodes, playerPosition, out)) return false;
			break;

		default:
			break;
		}
	}

	return true;
}

bool InteractableSystem::PassesVehicleFilter(const ObjectNode& node,
	const std::unordered_map<uint32_t, ObjectNode>& nodes,
	const float playerPosition[3],
	AIInteractable& out) const
{
	// --- 1. Fetch static seat definitions from the vehi tag -------------------

	//VehiData vehiData;
	//const bool hasVehiData =
	//	g_pState->Domain->MapVehi->GetVehi(node.TagName, vehiData);

	// --- 2. Collect bipeds currently mounted on this vehicle ------------------
	//
	// CollectDirectBipeds only traverses children of THIS node (the vehi parent).
	// Bipeds inside a child vehi (e.g. the Warthog turret) are NOT included here;
	// they will be collected when that child vehi is processed as its own entry.

	const std::vector<uint32_t> occupants =
		this->CollectDirectBipeds(node, nodes);

	const int32_t occupantCount = static_cast<int32_t>(occupants.size());

	// --- 3. Determine whether the vehicle has a free seat ---------------------
	//
	// With real tag data: exact comparison against the defined seat count.
	// Without tag data: fall back to the original child-counting heuristic.

	bool hasFreeSeat = false;

	//if (hasVehiData && !vehiData.Seats.empty())
	//{
	//	hasFreeSeat = occupantCount < vehiData.SeatCount();
	//}
	//else
	//{
	//	// Fallback heuristic: count all direct children and compare.
	//	int32_t totalChildren = 0;
	//	uint32_t cursor = node.ChildHandle;
	//	while (cursor != 0xFFFFFFFF)
	//	{
	//		++totalChildren;
	//		auto it = nodes.find(cursor);
	//		if (it == nodes.end()) break;
	//		cursor = it->second.NextSiblingHandle;
	//	}
	//	hasFreeSeat = occupantCount < totalChildren || occupants.empty();
	//}

	if (!hasFreeSeat) return false;

	// --- 4. Build SeatStatus list ---------------------------------------------
	//
	// IMPORTANT: The engine graph does not expose which seat a biped occupies.
	// Two bipeds on a Warthog (driver + passenger) are indistinguishable —
	// both appear as direct children of the vehi parent with no seat index.
	//
	// What we CAN model accurately:
	//   - The total number of seats (from the tag).
	//   - Which seats are free vs occupied (by count, not by identity).
	//   - The semantic type of each seat definition (Driver/Passenger/Gunner).
	//   - Which specific biped is in the gunner seat of a turret child vehi
	//     (handled when that child vehi is its own interactable entry).
	//
	// What we CANNOT model without runtime memory reads:
	//   - Whether a specific biped is in the Driver seat vs the Passenger seat.
	//
	// Strategy: emit one SeatStatus per tag-defined seat in order. Mark the
	// first N seats as occupied (N = biped count) without claiming to know
	// WHICH specific seat each biped is in. The SeatName reflects the tag
	// definition, not a confirmed assignment.

	//if (hasVehiData && !vehiData.Seats.empty())
	//{
	//	for (int32_t i = 0; i < vehiData.SeatCount(); ++i)
	//	{
	//		const VehiSeatData& seatDef = vehiData.Seats[i];

	//		SeatStatus status;
	//		status.SeatName = AISeatTypeToString(seatDef.Type);
	//		status.AISeatType = seatDef.Type;
	//		status.IsOccupied = (i < occupantCount);
	//		// We assign a biped handle positionally only as a best-effort hint.
	//		// Do not rely on this for Driver-vs-Passenger discrimination.
	//		status.OccupyingBipedHandle =
	//			status.IsOccupied ? occupants[i] : 0xFFFFFFFF;

	//		// Seat world position: rotate the model-space local offset by the
	//		// vehicle's current orientation, then add to vehicle world position.
	//		//
	//		// Halo Reach axis convention (model space → world space):
	//		//   LocalPosition[0] (X) → Right   = Cross(Forward, Up)
	//		//   LocalPosition[1] (Y) → Forward
	//		//   LocalPosition[2] (Z) → Up
	//		//
	//		// node.Forward and node.Up are world-space orientation vectors
	//		// read from the object graph each tick, so this is always current.
	//		// node.Position is only valid for objects with no parent (ParentHandle
	//		// == 0xFFFFFFFF) — child objects have a broken world position near
	//		// zero. PassesVehicleFilter is only called for vehicle root nodes,
	//		// so this is safe.
	//		if (seatDef.HasLocalPosition)
	//		{
	//			const float fx = node.Forward[0], fy = node.Forward[1], fz = node.Forward[2];
	//			const float ux = node.Up[0], uy = node.Up[1], uz = node.Up[2];

	//			// Right = Cross(Forward, Up)
	//			const float rx = fy * uz - fz * uy;
	//			const float ry = fz * ux - fx * uz;
	//			const float rz = fx * uy - fy * ux;

	//			const float lx = seatDef.LocalPosition[0];
	//			const float ly = seatDef.LocalPosition[1];
	//			const float lz = seatDef.LocalPosition[2];

	//			status.SeatWorldPosition[0] = node.Position[0] + lx * rx + ly * fx + lz * ux;
	//			status.SeatWorldPosition[1] = node.Position[1] + lx * ry + ly * fy + lz * uy;
	//			status.SeatWorldPosition[2] = node.Position[2] + lx * rz + ly * fz + lz * uz;
	//		}
	//		else
	//		{
	//			// Marker not resolved — fall back to vehicle root position.
	//			status.SeatWorldPosition[0] = node.Position[0];
	//			status.SeatWorldPosition[1] = node.Position[1];
	//			status.SeatWorldPosition[2] = node.Position[2];
	//		}

	//		// TODO: This is not working correctly, vehi childs of vehi considers their bugged world position.
	//		// Distance from the AI player to this specific seat.
	//		{
	//			const float dx = status.SeatWorldPosition[0] - playerPosition[0];
	//			const float dy = status.SeatWorldPosition[1] - playerPosition[1];
	//			const float dz = status.SeatWorldPosition[2] - playerPosition[2];
	//			status.DistanceToPlayer = std::sqrt(dx * dx + dy * dy + dz * dz);
	//		}

	//		out.Seats.push_back(status);
	//	}
	//}
	//else
	//{
	//	// Fallback: reproduce the old behaviour for unresolved vehicles.
	//	for (uint32_t bipedHandle : occupants)
	//	{
	//		SeatStatus status;
	//		status.SeatName = "Seat";
	//		status.AISeatType = AISeatType::None;
	//		status.IsOccupied = true;
	//		status.OccupyingBipedHandle = bipedHandle;
	//		status.SeatWorldPosition[0] = node.Position[0];
	//		status.SeatWorldPosition[1] = node.Position[1];
	//		status.SeatWorldPosition[2] = node.Position[2];

	//		const float dx = status.SeatWorldPosition[0] - playerPosition[0];
	//		const float dy = status.SeatWorldPosition[1] - playerPosition[1];
	//		const float dz = status.SeatWorldPosition[2] - playerPosition[2];
	//		status.DistanceToPlayer = std::sqrt(dx * dx + dy * dy + dz * dz);

	//		out.Seats.push_back(status);
	//	}
	//}

	// --- 5. Interactable root position ----------------------------------------

	out.Position[0] = node.Position[0];
	out.Position[1] = node.Position[1];
	out.Position[2] = node.Position[2];

	return true;
}



std::vector<uint32_t> InteractableSystem::CollectDirectBipeds(
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