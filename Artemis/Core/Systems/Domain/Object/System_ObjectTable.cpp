#include "pch.h"

// Header.
#include "System_ObjectTable.h"

// Hooks.
#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Domain/Core_Hook_Domain.h"

// Object.
#include "Core/Hooks/Domain/Object/Hook_ObjectTable.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

// Map.
#include "Core/States/Domain/Map/Bipd/State_MapBipd.h"
#include "Core/States/Domain/Map/Bloc/State_MapBloc.h"
#include "Core/States/Domain/Map/Coll/State_MapColl.h"
#include "Core/States/Domain/Map/Eqip/State_MapEqip.h"
#include "Core/States/Domain/Map/Jmad/State_MapJmad.h"
#include "Core/States/Domain/Map/Mach/State_MapMach.h"
#include "Core/States/Domain/Map/Mode/State_MapMode.h"
#include "Core/States/Domain/Map/Phmo/State_MapPhmo.h"
#include "Core/States/Domain/Map/Proj/State_MapProj.h"
#include "Core/States/Domain/Map/Scen/State_MapScen.h"
#include "Core/States/Domain/Map/Scnr/State_MapScnr.h"
#include "Core/States/Domain/Map/Vehi/State_MapVehi.h"
#include "Core/States/Domain/Map/Weap/State_MapWeap.h"
#include "Core/States/Domain/Map/Ctrl/State_MapCtrl.h"

// Object.
#include "Core/States/Domain/Object/State_ObjectTable.h"

// Systems.
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

// Map.
#include "../Map/System_Map.h"

// Debug.
#include "Core/Systems/Interface/System_Debug.h"

void System_ObjectTable::OnObjectCreated(uint32_t handle, uint32_t datumIndex)
{
	auto& map = *g_pSystem->Domain->Map;
	auto& debug = *g_pSystem->Debug;

	// Determines whether this datum index has a valid tag reference.
	auto info = map.ResolveHandle(datumIndex);
	if (!info.IsValid)
	{
		debug.Log("[ObjectTableSystem] WARNING:Failed to resolve"
			" DatumIndex 0x%X for handle 0x%X.", datumIndex, handle);
		return;
	}

	LiveObject object;

	// Allows to find this object memory address inside the object table.
	object.Handle = handle;

	// Allows to indetify the specific object inside the opened map.
	object.DatumIndex = datumIndex;

	// Tag Group of this object.
	object.Class = info.Class;

	// Tag Name of this object.
	object.TagName = info.Name;

	// Filters invalid objects.
	if (object.Class == "" || object.TagName == "")
	{
		g_pSystem->Debug->Log("[ObjectTableSystem] WARNING:" 
			" Invalid object created, skipping.");
		return;
	}

	// Placeholders, filled inside of 'System_ObjectTable::UpdateObjectTable()'. 
	object.SpecificObject = [&]() -> SpecificObject {
		if (object.Class == "bipd") return BipedObject{};
		if (object.Class == "weap") return WeaponObject{};
		if (object.Class == "vehi") return VehicleObject{};
		if (object.Class == "eqip") return EquipmentObject{};
		return std::monostate{};
	}();

	// Define the object's profile.
	ObjectProfile profile;
	profile.PrimaryClass = this->ClassNameToObjectClass(object.Class);

	profile.HasBipd = g_pState->Domain->MapBipd->HasBipd(object.TagName);
	profile.HasBloc = g_pState->Domain->MapBloc->HasBloc(object.TagName);
	profile.HasColl = g_pState->Domain->MapColl->HasColl(object.TagName);
	profile.HasCtrl = g_pState->Domain->MapCtrl->HasCtrl(object.TagName);
	profile.HasEqip = g_pState->Domain->MapEqip->HasEqip(object.TagName);
	profile.HasJmad = g_pState->Domain->MapJmad->HasJmad(object.TagName);
	profile.HasMach = g_pState->Domain->MapMach->HasMach(object.TagName);
	profile.HasMode = g_pState->Domain->MapMode->HasMode(object.TagName);
	profile.HasPhmo = g_pState->Domain->MapPhmo->HasPhmo(object.TagName);
	profile.HasProj = g_pState->Domain->MapProj->HasProj(object.TagName);
	profile.HasScen = g_pState->Domain->MapScen->HasScen(object.TagName);
	profile.HasScnr = g_pState->Domain->MapScnr->HasScnr(object.TagName);
	profile.HasVehi = g_pState->Domain->MapVehi->HasVehi(object.TagName);
	profile.HasWeap = g_pState->Domain->MapWeap->HasWeap(object.TagName);

	object.Profile = profile;

	g_pState->Domain->ObjectTable->AddObject(handle, object);
}

// Called from 'Hook_ReleaseObject::HookedReleaseObject', its responsible
// to delete the released objects by the game engine. With this we ensure
// that the Artemis object table has the same objects that the game engine's 
// object table has.
void System_ObjectTable::OnObjectDestroyed(uint32_t handle)
{
	auto deletedObject = g_pState->Domain->ObjectTable->RemoveObject(handle);
	if (!deletedObject.has_value())
	{
		g_pSystem->Debug->Log("[ObjectTableSystem] WARNING:"
			" OnObjectDestroyed called for unknown handle 0x%X.", handle);
	}
}

// Called from 'Thread_AI::Run' once a map has been loaded. Its responsible 
// of updating all the Artemis object table, to obtain the latest objects data 
// from the game engine's object table.
void System_ObjectTable::UpdateObjectTable()
{
	// Gets the memory address of the game engine's object table.
	uintptr_t tableBase = g_pState->Domain->ObjectTable->GetObjectTableBase();
	if (tableBase == 0) return;

	g_pState->Domain->ObjectTable->UpdateObjects(
		[&](uint32_t handle, LiveObject& object) {
		// 1. Index = Handle & 0xFFFF
		uint32_t index = handle & 0xFFFF;
	
		// 2. Offset = Index * 0x18
		uintptr_t offset = (uintptr_t)index * 0x18;
	
		// 3. Address = ObjectTableBase + Offset
		uintptr_t entryAddr = tableBase + offset;

		if (entryAddr != 0)
		{
			uint32_t tableHandle = *(uint32_t*)(entryAddr);

			// Each table entry has the object type at the offset '0x04'.
			ObjectClass objectType = *(ObjectClass*)(entryAddr + 0x04);

			// Each table entry has the pointer to the object header,
			// which contains all the data of this object.
			uintptr_t entityPtr = *(uintptr_t*)(entryAddr + 0x10);

			// The object table only uses the 'Salt' of the handle.
			uint16_t tableSalt = (uint16_t)(tableHandle & 0xFFFF);
			uint16_t expectedSalt = (uint16_t)(handle >> 16);

			// Filter: Is this table entry, the expected object we were looking for?
			if (entityPtr != 0 && tableSalt == expectedSalt)
			{
				// Object addresses can change, so we make sure to always update it.
				if (object.Address != entityPtr) object.Address = entityPtr;

				// We assign the object type to this object, only if it haven't been assigned.
				// Object types cannot change once the object was created.
				if (object.Type == ObjectClass::Invalid &&
					objectType != ObjectClass::Invalid) object.Type = objectType;
		
				// Update the more, specific data of this object.
				this->UpdateObjectData(object);
			}
			else object.Address = 0;
		}
	});
}

void System_ObjectTable::FindObjectTableBase()
{
	uintptr_t tableBase = g_pState->Domain->ObjectTable->GetObjectTableBase();
	if (tableBase == 0)
	{
		tableBase = g_pHook->Domain->ObjectTable->GetObjectTable();
		if (!tableBase)
		{
			g_pSystem->Debug->Log("[ObjectTableSystem] ERROR:"
				" ObjectTableBase invalid.");
			return;
		}

		g_pSystem->Debug->Log("[ObjectTableSystem] INFO: ObjectTable: 0x%llX", tableBase);

		g_pState->Domain->ObjectTable->SetObjectTableBase(tableBase);
	}
}

void System_ObjectTable::Cleanup()
{
	g_pState->Domain->ObjectTable->Cleanup();
	g_pSystem->Debug->Log("[ObjectTableSystem] INFO: Cleanup completed.");
}

// Responsible of updating all the general & specific data of a given object.
void System_ObjectTable::UpdateObjectData(LiveObject& object)
{
	if (object.Address == 0) return;

	using namespace ObjectOffsets;

	auto& reader = *g_pSystem->Infrastructure->MemoryReader;

	// Update the parent-child-sibling handles.
	object.NextSiblingHandle = reader.Read<uint32_t>(object.Address, NextSiblingHandle);
	object.ChildHandle = reader.Read<uint32_t>(object.Address, ChildHandle);
	object.ParentHandle = reader.Read<uint32_t>(object.Address, ParentHandle);
	object.CurrentRadius = reader.Read<float>(object.Address, CurrentRadius);

	// Update the world-space position, and forward/up.
	object.Position = reader.ReadArray<float, 3>(object.Address, CurrentPosition);
	object.Forward = reader.ReadArray<float, 3>(object.Address, Forward);
	object.Up = reader.ReadArray<float, 3>(object.Address, Up);

	// Update the velocities.
	object.LinearVelocity = reader.ReadArray<float, 3>(object.Address, LinearVelocity);
	object.AngularVelocity = reader.ReadArray<float, 3>(object.Address, AngularVelocity);
	object.DamageReceived = reader.Read<float>(object.Address, DamageReceived);

	// Update the specific data.
	switch (object.Type)
	{
	case ObjectClass::Biped:
	{
		this->UpdateBiped(reader, object);
		break;
	}
		
	case ObjectClass::Vehicle:
	{
		this->UpdateVehicle(reader, object);
		break;
	}

	case ObjectClass::Weapon:
	{
		this->UpdateWeapon(reader, object);
		break;
	}

	case ObjectClass::Equipment:
	{
		this->UpdateEquipment(reader, object);
		break;
	}

	case ObjectClass::Projectile:
	{
		this->UpdateProjectiles(reader, object);
		break;
	}

	case ObjectClass::Scenery:
	{
		this->UpdateScenery(reader, object);
		break;
	}

	// TODO: Device machine...
	// TODO: Device control...

	case ObjectClass::Crate:
	{
		this->UpdateCrate(reader, object);
		break;
	}

	default:
		object.SpecificObject = std::monostate{};
		break;
	}
}	

void System_ObjectTable::UpdateBiped(System_MemoryReader& reader, LiveObject& object)
{
	namespace Biped = ObjectOffsets::Biped;

	BipedObject biped{};

	// State-related.
	biped.IsCrouched = reader.Read<uint8_t>(object.Address, Biped::IsCrouched);
	biped.IsGrounded = reader.Read<uint8_t>(object.Address, Biped::IsGrounded);
	biped.IsAbilityActive = reader.Read<uint8_t>(object.Address, Biped::IsAbilityActive);
	biped.ZoomLevel = reader.Read<ZoomLevel>(object.Address, Biped::ZoomLevel);

	// Movement-related.
	biped.MovementDirection = reader.ReadArray<float, 2>(object.Address, Biped::MovementDirection);
	biped.SurfaceNormal = reader.ReadArray<float, 3>(object.Address, Biped::SurfaceNormal);
	biped.GroundObjectHandle = reader.Read<uint32_t>(object.Address, Biped::GroundObjectHandle);
	biped.MaterialIndex = reader.Read<uint16_t>(object.Address, Biped::MaterialIndex);

	// Damager.
	biped.DamagerBipedHandle = reader.Read<uint32_t>(object.Address, Biped::DamagerBipedHandle);
	biped.DamagerPlayerHandle = reader.Read<uint32_t>(object.Address, Biped::DamagerPlayerHandle);

	// Health-related.
	if (TagName::Biped::IsSpartan(object.TagName))
	{
		biped.HealthRegenerationDelay = reader.Read<uint16_t>(object.Address, Biped::Spartan::HealthRegenerationDelay);
		biped.Health = reader.Read<float>(object.Address, Biped::Spartan::Health);
		biped.HasNoShields = reader.Read<uint8_t>(object.Address, Biped::Spartan::HasNoShields);
		biped.ShieldsRegenerationDelay = reader.Read<uint16_t>(object.Address, Biped::Spartan::ShieldsRegenerationDelay);
		biped.Shields = reader.Read<float>(object.Address, Biped::Spartan::Shields);
	}
	if (TagName::Biped::IsElite(object.TagName))
	{
		biped.HealthRegenerationDelay = reader.Read<uint16_t>(object.Address, Biped::Elite::HealthRegenerationDelay);
		biped.Health = reader.Read<float>(object.Address, Biped::Elite::Health);
		biped.HasNoShields = reader.Read<uint8_t>(object.Address, Biped::Elite::HasNoShields);
		biped.ShieldsRegenerationDelay = reader.Read<uint16_t>(object.Address, Biped::Elite::ShieldsRegenerationDelay);
		biped.Shields = reader.Read<float>(object.Address, Biped::Elite::Shields);
	}

	object.SpecificObject = biped;
}

void System_ObjectTable::UpdateVehicle(System_MemoryReader& reader, LiveObject& object)
{
	namespace Vehicle = ObjectOffsets::Vehicle;
	namespace Helpers = VehicleObjectHelpers;

	VehicleObject vehicle{};

	vehicle.Base = object.Address;
	vehicle.Type = Helpers::ResolveVehicleType(object.TagName);
	vehicle.SeatLayout = Helpers::GetSeatLayout(vehicle.Type);

	if (auto offsets = Helpers::ResolveBaseOffsets(vehicle.Type))
	{
		vehicle.RegenerationDelay = reader.Read<uint8_t>(object.Address, offsets->RegenerationDelay);
		vehicle.Health = reader.Read<float>(object.Address, offsets->Health);
	}

	if (TagName::Vehicle::HasBoost(object.TagName))
	{
		vehicle.BoostThrottle = reader.Read<float>(object.Address, Vehicle::BoostThrottle);
		vehicle.BoostEnergy = reader.Read<float>(object.Address, Vehicle::BoostEnergy);
		vehicle.BoostCooldown = reader.Read<float>(object.Address, Vehicle::BoostCooldown);
	}

	object.SpecificObject = vehicle;
}

void System_ObjectTable::UpdateWeapon(System_MemoryReader& reader, LiveObject& object)
{
	namespace Weapon = ObjectOffsets::Weapon;

	WeaponObject weapon{};

	// Ammo-related.
	weapon.TotalHeat = reader.Read<float>(object.Address, Weapon::TotalHeat);
	weapon.TotalEnergy = 1.0f - reader.Read<float>(object.Address, Weapon::TotalEnergy);
	weapon.TotalAmmo = reader.Read<uint16_t>(object.Address, Weapon::TotalAmmo);
	weapon.CurrentAmmo = reader.Read<uint16_t>(object.Address, Weapon::CurrentAmmo);

	// State-related.
	weapon.IsFiring = reader.Read<uint8_t>(object.Address, Weapon::IsFiring);
	weapon.IsReloading = reader.Read<uint8_t>(object.Address, Weapon::IsReloading);
	weapon.ChargeProgress = reader.Read<uint8_t>(object.Address, Weapon::ChargeProgress);
	weapon.ActionState = reader.Read<ActionState>(object.Address, Weapon::ActionState);

	// Team.
	if (object.TagName == TagName::Objective::Flag)
	{
		weapon.Team = reader.Read<Team>(object.Address, Weapon::Flag::Team);
	}
	else if (object.TagName == TagName::Objective::Bomb)
	{
		weapon.Team = reader.Read<Team>(object.Address, Weapon::Bomb::Team);
	}

	object.SpecificObject = weapon;
}

void System_ObjectTable::UpdateEquipment(System_MemoryReader& reader, LiveObject& object)
{
	namespace Equipment = ObjectOffsets::Equipment;

	EquipmentObject equipment{};

	equipment.TotalEnergy = reader.Read<float>(object.Address, Equipment::TotalEnergy);

	object.SpecificObject = equipment;
}

void System_ObjectTable::UpdateProjectiles(System_MemoryReader& reader, LiveObject& object)
{
	namespace Projectile = ObjectOffsets::Projectile;

	ProjectileObject projectile{};

	projectile.ProjectileFlags = reader.Read<ProjectileFlags>(object.Address, Projectile::RuntimeFlags);
	projectile.OwnerBipedHandle = reader.Read<uint32_t>(object.Address, Projectile::OwnerBipedHandle);
	projectile.OwnerWeaponHandle = reader.Read<uint32_t>(object.Address, Projectile::OwnerWeaponHandle);

	object.SpecificObject = projectile;
}

void System_ObjectTable::UpdateCrate(System_MemoryReader& reader, LiveObject& object)
{
	namespace Crate = ObjectOffsets::Crate;
	namespace Helpers = CrateObjectHelpers;

	CrateObject crate{};

	crate.Base = object.Address;
	crate.Type = Helpers::ResolveCrateType(object.TagName);

	// Zone types. (FlagStand, CapturePlate, HillMarker)
	if (auto offsets = Helpers::ResolveZoneOffsets(crate.Type))
	{
		ZoneData zone{};

		zone.Shape.Radius = reader.Read<float>(object.Address, offsets->Radius);
		zone.Shape.Length = reader.Read<float>(object.Address, offsets->Length);
		zone.Shape.Top = reader.Read<float>(object.Address, offsets->Top);
		zone.Shape.Bottom = reader.Read<float>(object.Address, offsets->Bottom);
		zone.Shape.ShapeType = reader.Read<ShapeType>(object.Address, offsets->ShapeType);
		zone.Team = reader.Read<Team>(object.Address, offsets->Team);

		crate.Zone = zone;
	}

	// Teleporter.
	else if (crate.Type == CrateType::TeleporterSender ||
		crate.Type == CrateType::TeleporterReceiver ||
		crate.Type == CrateType::TeleporterTwoWay)
	{
		TeleporterData teleport{};

		teleport.ZoneShape.Radius = reader.Read<float>(object.Address, Crate::Teleporter::Radius);
		teleport.ZoneShape.Length = reader.Read<float>(object.Address, Crate::Teleporter::Lenght);
		teleport.ZoneShape.Top = reader.Read<float>(object.Address, Crate::Teleporter::Top);
		teleport.ZoneShape.Bottom = reader.Read<float>(object.Address, Crate::Teleporter::Bottom);
		teleport.ZoneShape.ShapeType = reader.Read<ShapeType>(object.Address, Crate::Teleporter::ShapeType);
		teleport.Channel = reader.Read<uint8_t>(object.Address, Crate::Teleporter::Channel);
		teleport.AllowedObjects = reader.Read<AllowedObjects>(object.Address, Crate::Teleporter::AllowedObjects);

		crate.Teleporter = teleport;
	}

	// Destructibles.
	else if (auto offsets = Helpers::ResolveDestructibleOffsets(crate.Type))
	{
		DestructibleData destructible{};
		destructible.Health = reader.Read<float>(object.Address, offsets->Health);

		if (offsets->RegenerationDelay.has_value())
		{
			destructible.RegenerationDelay = reader.Read<uint16_t>(object.Address, *offsets->RegenerationDelay);
		}

		crate.Destructible = destructible;
	}

	// Lifts.
	else if (crate.Type == CrateType::Lift)
	{
		LiftData lift{};

		// Angle Type.
		if (TagName::Lift::IsCurved(object.TagName))
		{
			lift.AngleType = AngleType::Curved;
		}
		else if (TagName::Lift::IsVertical(object.TagName))
		{
			lift.AngleType = AngleType::Vertical;
		}
		else if (TagName::Lift::IsRedirected(object.TagName))
		{
			lift.AngleType = AngleType::Redirected;
		}

		// Force Type.
		if (TagName::Lift::IsDefault(object.TagName))
		{
			lift.ForceType = ForceType::Default;
		}
		else if (TagName::Lift::IsLight(object.TagName))
		{
			lift.ForceType = ForceType::Light;
		}
		else if (TagName::Lift::IsHeavy(object.TagName))
		{
			lift.ForceType = ForceType::Heavy;
		}
		else if (TagName::Lift::IsVehicle(object.TagName))
		{
			lift.ForceType = ForceType::Vehicle;
		}

		crate.Lift = lift;
	}

	// Shields.
	else if (crate.Type == CrateType::Shield)
	{
		ShieldData shield{};

		if (TagName::Shield::IsOneWay(object.TagName))
		{
			shield.ShieldType = ShieldType::OneWay;
		}
		else if (TagName::Shield::IsTwoWay(object.TagName))
		{
			shield.ShieldType = ShieldType::TwoWay;

			if (TagName::Shield::IsShieldDoor(object.TagName))
			{
				shield.IsShieldDoor = true;
			}
		}
		else if (TagName::Shield::IsBlocker(object.TagName))
		{
			shield.ShieldType = ShieldType::Blocker;
		}

		crate.Shield = shield;
	}

	object.SpecificObject = crate;
}

void System_ObjectTable::UpdateScenery(System_MemoryReader& reader, LiveObject& object)
{
	namespace Scenery = ObjectOffsets::Scenery;
	namespace Helpers = SceneryObjectHelpers;

	SceneryObject scenery{};
	scenery.Base = object.Address;
	scenery.Type = Helpers::ResolveSceneryType(object.TagName);

	// Spawn Points.
	if (Helpers::IsSpawnPoint(scenery.Type))
	{
		SpawnData spawn{};

		if (scenery.Type == SceneryType::InvisibleRespawnPoint)
		{
			spawn.Team = Team::Neutral;
		}
		else
		{
			spawn.Team = reader.Read<Team>(
				object.Address, Scenery::SpawnPoint::Team);
		}

		scenery.Spawn = spawn;
	}

	// Boundaries.
	else if (Helpers::IsBoundary(scenery.Type))
	{
		BoundaryData boundary{};

		boundary.Shape.Radius = reader.Read<float>(object.Address, Scenery::Boundary::Radius);
		boundary.Shape.Length = reader.Read<float>(object.Address, Scenery::Boundary::Lenght);
		boundary.Shape.Top = reader.Read<float>(object.Address, Scenery::Boundary::Top);
		boundary.Shape.Bottom = reader.Read<float>(object.Address, Scenery::Boundary::Bottom);
		boundary.Shape.ShapeType = reader.Read<ShapeType>(object.Address, Scenery::Boundary::ShapeType);
		boundary.Team = reader.Read<Team>(object.Address, Scenery::Boundary::Team);

		scenery.Boundary = boundary;
	}

	// Portable Shield.
	else if (Helpers::IsPortableShield(scenery.Type))
	{
		PortableShieldData portable{};

		portable.RegenerationDelay = reader.Read<uint16_t>(object.Address, Scenery::PortableShield::RegenerationDelay);
		portable.Health = reader.Read<float>(object.Address, Scenery::PortableShield::Health);

		scenery.PortableShield = portable;
	}

	object.SpecificObject = scenery;
}

// --- Helpers ---

ObjectClass System_ObjectTable::ClassNameToObjectClass(const std::string& className)
{
	static const std::unordered_map<std::string, ObjectClass> k_ObjectClassMap =
	{
		{ "bloc", ObjectClass::Crate },
		{ "mach", ObjectClass::DeviceMachine },
		{ "sbsp", ObjectClass::ScenarioStructureBsp },
		{ "scen", ObjectClass::Scenery },
		{ "bipd", ObjectClass::Biped },
		{ "coll", ObjectClass::CollisionModel },
		{ "mode", ObjectClass::RenderModel },
		{ "phmo", ObjectClass::PhysicsModel },
		{ "scnr", ObjectClass::Scenario },
		{ "ctrl", ObjectClass::DeviceControl },
		{ "eqip", ObjectClass::Equipment},
		{ "proj", ObjectClass::Projectile },
		{ "vehi", ObjectClass::Vehicle },
		{ "weap", ObjectClass::Weapon },
	};

	auto it = k_ObjectClassMap.find(className);
	return it != k_ObjectClassMap.end() ? it->second : ObjectClass::Invalid;
}