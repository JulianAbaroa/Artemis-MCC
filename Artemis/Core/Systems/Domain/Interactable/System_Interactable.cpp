#include "pch.h"

// Header.
#include "System_Interactable.h"

// Types.
#include "Core/Types/Domain/Map/MapMagics.h"
#include "Core/Types/Domain/Environment/ModeGeometry.h"

// States.
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

// Map.
#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Map/Vehi/State_MapVehi.h"
#include "Core/States/Domain/Map/Eqip/State_MapEqip.h"
#include "Core/States/Domain/Map/Weap/State_MapWeap.h"
#include "Core/States/Domain/Map/Proj/State_MapProj.h"
#include "Core/States/Domain/Map/Ctrl/State_MapCtrl.h"

// Object.
#include "Core/States/Domain/Object/State_ObjectTable.h"

// Player.
#include "Core/States/Domain/Player/State_PlayerTable.h"

// Intraction
#include "Core/States/Domain/Interaction/State_InteractionTable.h"

// Classification.
#include "Core/States/Domain/Classification/State_Classification.h"

// Graph.
#include "Core/States/Domain/Graph/State_ObjectGraph.h"
#include "Core/States/Domain/Graph/State_PlayerGraph.h"

// Environment.
#include "Core/States/Domain/Environment/State_Environment.h"

// Interactable.
#include "Core/States/Domain/Interactable/State_Interactable.h"

// --- Systems ---
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"

// Classification.
#include "Core/Systems/Domain/Classification/System_ObjectClassifier.h"

// Interactable.
#include "Core/Systems/Domain/Interactable/Vehi/System_VehiDataBuilder.h"
#include "Core/Systems/Domain/Interactable/Eqip/System_EqipDataBuilder.h"
#include "Core/Systems/Domain/Interactable/Weap/System_WeapDataBuilder.h"
#include "Core/Systems/Domain/Interactable/Proj/System_ProjDataBuilder.h"
#include "Core/Systems/Domain/Interactable/Ctrl/System_CtrlDataBuilder.h"

#include "Core/Systems/Interface/System_Debug.h"

#include <algorithm>
#include <cmath>

// --- BuildForMap ---

void System_Interactable::BuildForMap()
{
	const int32_t tagCount =
		static_cast<int32_t>(g_pState->Domain->Map->GetTagsSize());

	int32_t vehiCount = 0;
	int32_t eqipCount = 0;
	int32_t weapCount = 0;
	int32_t projCount = 0;
	int32_t ctrlCount = 0;

	for (int32_t i = 0; i < tagCount; ++i)
	{
		const Map_TagTableEntry& entry = g_pState->Domain->Map->GetTag(i);
		if (entry.TagGroupIndex < 0) continue;

		const uint32_t magic =
			g_pState->Domain->Map->GetGroupMagic(entry.TagGroupIndex);

		const std::string tagName = g_pState->Domain->Map->GetTagName(i);
		if (tagName.empty()) continue;

		if (magic == MapMagics::k_VehiMagic)
		{
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
			++vehiCount;
		}
		else if (magic == MapMagics::k_EqipMagic)
		{
			const EqipObject* eqip = g_pState->Domain->MapEqip->GetEqip(tagName);
			if (!eqip)
			{
				g_pSystem->Debug->Log("[InteractableSystem] WARNING:"
					" Eqip tag found in table but not loaded: ", tagName);
				continue;
			}

			EquipmentData data =
				g_pSystem->Domain->EqipDataBuilder->BuildData(*eqip);

			g_pState->Domain->Interactable->AddEquipmentData(tagName, std::move(data));
			++eqipCount;
		}
		else if (magic == MapMagics::k_WeapMagic)
		{
			const WeapObject* weap = g_pState->Domain->MapWeap->GetWeap(tagName);
			if (!weap)
			{
				g_pSystem->Debug->Log("[InteractableSystem] WARNING:"
					" Weap tag found in table but not loaded: ", tagName);
				continue;
			}

			WeaponData data =
				g_pSystem->Domain->WeapDataBuilder->BuildData(*weap);

			g_pState->Domain->Interactable->AddWeaponData(tagName, std::move(data));
			++weapCount;
		}
		else if (magic == MapMagics::k_ProjMagic)
		{
			const ProjObject* proj = g_pState->Domain->MapProj->GetProj(tagName);
			if (!proj)
			{
				g_pSystem->Debug->Log("[InteractableSystem] WARNING:"
					" Proj tag found in table but not loaded: ", tagName);
				continue;
			}

			ProjectileData data =
				g_pSystem->Domain->ProjDataBuilder->BuildData(*proj);

			g_pState->Domain->Interactable->AddProjectileData(tagName, std::move(data));
			++projCount;
		}
		else if (magic == MapMagics::k_CtrlMagic)
		{
			const CtrlObject* ctrl = g_pState->Domain->MapCtrl->GetCtrl(tagName);
			if (!ctrl)
			{
				g_pSystem->Debug->Log("[InteractableSystem] WARNING:"
					" Proj tag found in table but not loaded: ", tagName);
				continue;
			}

			ControlDeviceData data =
				g_pSystem->Domain->CtrlDataBuilder->BuildData(*ctrl);

			g_pState->Domain->Interactable->AddControlDeviceData(tagName, std::move(data));
			++ctrlCount;
		}
	}

	g_pSystem->Debug->Log("[InteractableSystem] INFO: Interactable built."
		" Vehi: %d | Eqip: %d | Weap: %d | Proj: %d | Ctrl: %d",
		vehiCount, eqipCount, weapCount, projCount, ctrlCount);
}

// --- UpdateInteractables ---

void System_Interactable::UpdateInteractables()
{
	// --- Gather inputs ---
	uint32_t selfPlayerHandle =
		g_pState->Domain->PlayerTable->GetPlayerHandleByName("Artemis11010");

	const LivePlayer* selfPtr =
		g_pState->Domain->PlayerTable->GetPlayer(selfPlayerHandle);
	if (!selfPtr) return;

	const LivePlayer self = *selfPtr;
	if (!self.IsAlive)
	{
		g_pState->Domain->Interactable->ClearInteractables();
		return;
	}

	const LiveInteraction interaction =
		g_pState->Domain->InteractionTable->GetLiveInteraction();

	const std::vector<ClassifiedObject>& classified =
		g_pState->Domain->Classification->GetObjects();

	const std::unordered_map<uint32_t, ObjectNode>& nodes =
		g_pState->Domain->ObjectGraph->GetNodes();

	const std::vector<PlayerTree>& playerTrees =
		g_pState->Domain->PlayerGraph->GetTrees();

	const std::unordered_map<uint32_t, LiveObject>& objectTable =
		g_pState->Domain->ObjectTable->GetObjectTable();

	// --- Process each classified object ---
	std::vector<AIInteractable> results;
	results.reserve(32);

	for (const ClassifiedObject& classified : classified)
	{
		auto it = objectTable.find(classified.Handle);
		if (it == objectTable.end()) continue;

		const LiveObject& object = it->second;
		if (object.Address == 0) continue;

		AIInteractable interactable;

		switch (classified.Role)
		{
		case ObjectRole::Vehicle:
		{
			if (!this->BuildVehicleInteractable(
				object, self, interaction, nodes, playerTrees, objectTable,
				interactable))
			{
				continue;
			}
			break;
		}

		case ObjectRole::WeaponPickup:
		case ObjectRole::ArmorAbilityPickup:
		case ObjectRole::ObjectivePickup:
			interactable = this->BuildPickupInteractable(object, classified.Role, self, interaction);
			break;

		case ObjectRole::HealthStation:
			interactable = this->BuildInteractInteractable(object, self, interaction);
			break;

		case ObjectRole::AmmoPickup:
			interactable = this->BuildAmmoInteractable(object, self, classified.Role);
			break;

		default:
			continue;
		}

		results.push_back(std::move(interactable));
	}

	g_pState->Domain->Interactable->SetInteractables(std::move(results));
}

void System_Interactable::Cleanup()
{
	g_pState->Domain->Interactable->Cleanup();
	g_pSystem->Debug->Log("[InteractableSystem] INFO: Cleanup completed.");
}

// --- BuildVehicleInteractable ---

bool System_Interactable::BuildVehicleInteractable(
	const LiveObject& object,
	const LivePlayer& self,
	const LiveInteraction& interaction,
	const std::unordered_map<uint32_t, ObjectNode>& nodes,
	const std::vector<PlayerTree>& playerTrees,
	const std::unordered_map<uint32_t, LiveObject>& objectTable,
	AIInteractable& out) const
{
	// VehicleObject must be present in the variant.
	const VehicleObject* vehiPtr =
		std::get_if<VehicleObject>(&object.SpecificObject);
	if (!vehiPtr) return false;

	const VehicleObject& vehi = *vehiPtr;

	// Resolve seat statuses from memory offsets.
	// Returns false if no free non-hijacker seat exists.
	std::vector<SeatStatus> seats;
	if (!this->ResolveSeatStatuses(
		vehi, object, nodes, objectTable, playerTrees, self, seats))
	{
		return false;
	}

	// Collect VehiclePart child handles (torretas, etc.)
	std::vector<uint32_t> partHandles =
		this->CollectVehiclePartHandles(object.Handle, nodes, objectTable);

	out.Handle = object.Handle;
	out.Role = ObjectRole::Vehicle;
	out.Behaviors = this->DeriveBehaviors(ObjectRole::Vehicle);
	out.Activation = this->DeriveActivation(ObjectRole::Vehicle);
	out.Seats = std::move(seats);
	out.ChildHandles = std::move(partHandles);
	out.IsEngineSelected = (interaction.TargetObjectHandle == object.Handle);
	out.DistanceToPlayer = this->Distance(object.Position, self.WeaponPosition);

	return true;
}

// --- BuildPickupInteractable ---

AIInteractable System_Interactable::BuildPickupInteractable(
	const LiveObject& object, ObjectRole role,
	const LivePlayer& self, const LiveInteraction& interaction) const
{
	// Pickups attached to a parent (e.g. equipped weapon) are not pickups.
	// The classifier already handles this for WeaponPickup/EquipmentPickup,
	// but we guard here as a second layer.
	AIInteractable out;
	out.Handle = object.Handle;
	out.Role = role;
	out.Behaviors = this->DeriveBehaviors(role);
	out.Activation = this->DeriveActivation(role);
	out.DistanceToPlayer = this->Distance(object.Position, self.WeaponPosition);
	out.IsEngineSelected = (interaction.TargetObjectHandle == object.Handle);
	return out;
}

// --- BuildInteractInteractable ---

AIInteractable System_Interactable::BuildInteractInteractable(
	const LiveObject& object, const LivePlayer& self, 
	const LiveInteraction& interaction) const
{
	AIInteractable out;
	out.Handle = object.Handle;
	out.Role = ObjectRole::HealthStation;
	out.Behaviors = this->DeriveBehaviors(ObjectRole::HealthStation);
	out.Activation = this->DeriveActivation(ObjectRole::HealthStation);
	out.DistanceToPlayer = this->Distance(object.Position, self.WeaponPosition);
	out.IsEngineSelected = (interaction.TargetObjectHandle == object.Handle);
	return out;
}

AIInteractable System_Interactable::BuildAmmoInteractable(
	const LiveObject& object, const LivePlayer& self,
	ObjectRole role) const
{
	AIInteractable out;
	out.Handle = object.Handle;
	out.Role = role;
	out.Behaviors = this->DeriveBehaviors(role);
	out.Activation = this->DeriveActivation(role);
	out.DistanceToPlayer = this->Distance(object.Position, self.WeaponPosition);
	return out;
}

// --- ResolveSeatStatuses ---

bool System_Interactable::ResolveSeatStatuses(
	const VehicleObject& vehi,
	const LiveObject& object,
	const std::unordered_map<uint32_t, ObjectNode>& nodes,
	const std::unordered_map<uint32_t, LiveObject>& objectTable,
	const std::vector<PlayerTree>& playerTrees,
	const LivePlayer& self,
	std::vector<SeatStatus>& outSeats) const
{
	if (!vehi.SeatLayout) return false;

	// Collect biped children of this vehicle for occupant resolution.
	const std::vector<uint32_t> bipedChildren =
		this->CollectBipedChildHandles(object.Handle, nodes, objectTable);

	bool anyFreeSeat = false;
	bool anyFreeHijack = false;

	for (const SeatInfo& seatInfo : vehi.SeatLayout->seats)
	{
		SeatStatus seat;
		seat.SeatName = seatInfo.Name;
		seat.IsHijackerSlot = seatInfo.IsHijackerSlot;
		seat.IsOccupied = !vehi.IsSeatFree(seatInfo);

		// World position: vehicle root position as fallback.
		seat.SeatWorldPosition = object.Position;
		seat.DistanceToPlayer =
			this->Distance(seat.SeatWorldPosition, self.WeaponPosition);

		// --- Occupant resolution ---
		//
		// We can resolve OccupyingBipedHandle with confidence only in
		// specific cases. Otherwise we leave it at 0xFFFFFFFF.
		//
		if (seat.IsOccupied)
		{
			// Case 1: Vehicle has exactly one non-hijacker seat (Banshee,
			// Ghost, ShadeTurret). If occupied, the only biped child is the
			// occupant.
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
			// Case 2: Self player is in this vehicle — use LivePlayer data.
			// We check if any biped child belongs to self.
			else
			{
				for (uint32_t bipedHandle : bipedChildren)
				{
					if (bipedHandle == self.CurrentBipedHandle)
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

	// Also process VehiclePart children (torretas).
	// Each part has its own SeatLayout; if a biped is a child of the part,
	// we can resolve the occupant with certainty.
	const auto nodeIt = nodes.find(object.Handle);
	if (nodeIt != nodes.end())
	{
		for (uint32_t childHandle : nodeIt->second.ChildrenHandles)
		{
			auto childObjIt = objectTable.find(childHandle);
			if (childObjIt == objectTable.end()) continue;
			if (childObjIt->second.Type != ObjectClass::Vehicle) continue;

			const LiveObject& partObj = childObjIt->second;
			const VehicleObject* partVehi =
				std::get_if<VehicleObject>(&partObj.SpecificObject);
			if (!partVehi || !partVehi->SeatLayout) continue;

			// Collect biped children of this part.
			const std::vector<uint32_t> partBipeds =
				this->CollectBipedChildHandles(childHandle, nodes, objectTable);

			for (const SeatInfo& seatInfo : partVehi->SeatLayout->seats)
			{
				SeatStatus seat;
				seat.SeatName = seatInfo.Name;
				seat.IsHijackerSlot = (std::string(seatInfo.Name).find("Hijacker")
					!= std::string::npos);
				seat.IsOccupied = !partVehi->IsSeatFree(seatInfo);

				seat.SeatWorldPosition = partObj.Position;
				seat.DistanceToPlayer =
					this->Distance(seat.SeatWorldPosition, self.WeaponPosition);

				// Turret seats are 1:1 with their biped child → certainty.
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

std::vector<uint32_t> System_Interactable::CollectVehiclePartHandles(
	uint32_t vehicleHandle,
	const std::unordered_map<uint32_t, ObjectNode>& nodes,
	const std::unordered_map<uint32_t, LiveObject>& objectTable) const
{
	std::vector<uint32_t> parts;

	auto nodeIt = nodes.find(vehicleHandle);
	if (nodeIt == nodes.end()) return parts;

	for (uint32_t childHandle : nodeIt->second.ChildrenHandles)
	{
		auto objIt = objectTable.find(childHandle);
		if (objIt == objectTable.end()) continue;
		if (objIt->second.Type == ObjectClass::Vehicle)
			parts.push_back(childHandle);
	}

	return parts;
}


// --- CollectBipedChildHandles ---

std::vector<uint32_t> System_Interactable::CollectBipedChildHandles(
	uint32_t vehicleHandle,
	const std::unordered_map<uint32_t, ObjectNode>& nodes,
	const std::unordered_map<uint32_t, LiveObject>& objectTable) const
{
	std::vector<uint32_t> bipeds;

	auto nodeIt = nodes.find(vehicleHandle);
	if (nodeIt == nodes.end()) return bipeds;

	for (uint32_t childHandle : nodeIt->second.ChildrenHandles)
	{
		auto objIt = objectTable.find(childHandle);
		if (objIt == objectTable.end()) continue;
		if (objIt->second.Type == ObjectClass::Biped)
			bipeds.push_back(childHandle);
	}

	return bipeds;
}

// --- FindPlayerHandleForBiped ---

uint32_t System_Interactable::FindPlayerHandleForBiped(
	uint32_t bipedHandle,
	const std::vector<PlayerTree>& playerTrees) const
{
	for (const PlayerTree& tree : playerTrees)
	{
		if (tree.BipedHandle == bipedHandle)
			return tree.Handle;
	}
	return 0xFFFFFFFF;
}

// --- Shared helpers ---

float System_Interactable::Distance(const float a[3], const float b[3]) const
{
	float dx = a[0] - b[0];
	float dy = a[1] - b[1];
	float dz = a[2] - b[2];
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float System_Interactable::Distance(
	const std::array<float, 3>& a,
	const std::array<float, 3>& b) const
{
	float dx = a[0] - b[0];
	float dy = a[1] - b[1];
	float dz = a[2] - b[2];
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::vector<InteractableBehavior> System_Interactable::DeriveBehaviors(
	ObjectRole role) const
{
	std::vector<InteractableBehavior> behaviors;
	switch (role)
	{
	case ObjectRole::AmmoPickup:
	case ObjectRole::WeaponPickup:
	case ObjectRole::ArmorAbilityPickup:
		behaviors.push_back(InteractableBehavior::Pickup);
		break;
	case ObjectRole::Vehicle:
		behaviors.push_back(InteractableBehavior::EnterVehicle);
		break;
	case ObjectRole::HealthStation:
		behaviors.push_back(InteractableBehavior::Interact);
		break;
	default:
		break;
	}
	return behaviors;
}

InteractableActivation System_Interactable::DeriveActivation(
	ObjectRole role) const
{
	switch (role)
	{
	case ObjectRole::WeaponPickup:
	case ObjectRole::ArmorAbilityPickup:
	case ObjectRole::Vehicle:
	case ObjectRole::HealthStation:
		return InteractableActivation::KeyPress;

	case ObjectRole::AmmoPickup:
		return InteractableActivation::Proximity;
	default:
		return InteractableActivation::None;
	}
}