#include "pch.h"

// Header.
#include "System_ObjectTable.h"

// --- States ---

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

#include "Core/States/Domain/Object/State_ObjectTable.h"

// --- Systems ---

#include "Core/Systems/Domain/Map/System_Map.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_MemoryReader.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

// Called from 'Hook_CreateObject::HookedCreateObject', its responisble of
// adding every single object that's created in-game to the Artemis 
// object table, and settings its base data.
void System_ObjectTable::OnObjectCreated(uint32_t handle, uint32_t datumIndex)
{
	// Determines whether this datum index has a valid tag reference.
	auto info = m_Dependencies.System_Map.ResolveHandle(datumIndex);
	if (!info.IsValid)
	{
		m_Dependencies.System_Debug.Log("[ObjectTableSystem] WARNING:"
			" Failed to resolve DatumIndex 0x%X for handle 0x%X.", 
			datumIndex, handle);
		return;
	}

	LiveObject object;

	// Allows to find this object memory address inside the object table.
	object.Handle = handle;

	// Allows to indetify the specific object inside the opened map.
	object.DatumIndex = datumIndex;

	// Tag Group of this object.
	object.FourCC = info.FourCC;

	// Tag Name of this object.
	object.TagName = info.TagName;

	// Filters invalid objects.
	if (object.FourCC == "" || object.TagName == "")
	{
		m_Dependencies.System_Debug.Log("[ObjectTableSystem] WARNING:"
			" Invalid object created, skipping.");
		return;
	}

	// Define the object's profile.
	ObjectProfile profile;

	profile.HasBipd = m_Dependencies.State_MapBipd.HasBipd(object.TagName);
	profile.HasBloc = m_Dependencies.State_MapBloc.HasBloc(object.TagName);
	profile.HasColl = m_Dependencies.State_MapColl.HasColl(object.TagName);
	profile.HasCtrl = m_Dependencies.State_MapCtrl.HasCtrl(object.TagName);
	profile.HasEqip = m_Dependencies.State_MapEqip.HasEqip(object.TagName);
	profile.HasJmad = m_Dependencies.State_MapJmad.HasJmad(object.TagName);
	profile.HasMach = m_Dependencies.State_MapMach.HasMach(object.TagName);
	profile.HasMode = m_Dependencies.State_MapMode.HasMode(object.TagName);
	profile.HasPhmo = m_Dependencies.State_MapPhmo.HasPhmo(object.TagName);
	profile.HasProj = m_Dependencies.State_MapProj.HasProj(object.TagName);
	profile.HasScen = m_Dependencies.State_MapScen.HasScen(object.TagName);
	profile.HasScnr = m_Dependencies.State_MapScnr.HasScnr(object.TagName);
	profile.HasVehi = m_Dependencies.State_MapVehi.HasVehi(object.TagName);
	profile.HasWeap = m_Dependencies.State_MapWeap.HasWeap(object.TagName);

	object.Profile = profile;

	m_Dependencies.State_ObjectTable.AddObject(handle, object);
}

// Called from 'Hook_ReleaseObject::HookedReleaseObject', its responsible
// to delete the released objects by the game engine. With this we ensure
// that the Artemis object table has the same objects that the game engine's 
// object table has.
void System_ObjectTable::OnObjectDestroyed(uint32_t handle)
{
	auto deletedObject = m_Dependencies.State_ObjectTable.RemoveObject(handle);
	if (!deletedObject.has_value())
	{
		m_Dependencies.System_Debug.Log("[ObjectTableSystem] WARNING:"
			" OnObjectDestroyed called for unknown handle 0x%X.", handle);
	}
}

// Called from 'Thread_AI::Run' once a map has been loaded. Its responsible 
// of updating all the Artemis object table, to obtain the latest objects data 
// from the game engine's object table.
void System_ObjectTable::UpdateObjectTable()
{
	// Gets the memory address of the game engine's object table.
	uintptr_t tableBase = m_Dependencies.State_ObjectTable.GetObjectTableBase();
	if (tableBase == 0) return;

	m_Dependencies.State_ObjectTable.UpdateObjects(
		[&](uint32_t handle, LiveObject& object) {
		// Index = Handle & 0xFFFF
		uint32_t index = handle & 0xFFFF;
	
		// Offset = Index * 0x18
		uintptr_t offset = (uintptr_t)index * 0x18;
	
		// Address = ObjectTableBase + Offset
		uintptr_t entryAddr = tableBase + offset;

		if (entryAddr != 0)
		{
			// The Salt (generation validator) in this Object Entry.
			uint16_t tableSalt = *(uint16_t*)(entryAddr);

			// The expected salt, in our saved handle.
			uint16_t expectedSalt = (uint16_t)(handle >> 16);

			// The Object Data Ptr readed as a pointer.
			uintptr_t entityPtr = *(uintptr_t*)(entryAddr + 0x10);

			if (entityPtr != 0 && tableSalt == expectedSalt)
			{
				// Update the object address only if it changed.
				if (object.Address != entityPtr) object.Address = entityPtr;

				// Each object entry has the object class at the offset '0x04'.
				ObjectClass objectClass = *(ObjectClass*)(entryAddr + 0x04);

				// Set the Class if it is necessary.
				if (object.Profile.Class == ObjectClass::Invalid &&
					objectClass != ObjectClass::Invalid)
				{
					object.Profile.Class = objectClass;
				}
		
				this->UpdateObjectData(object);
			}
			else object.Address = 0;
		}
	});
}

void System_ObjectTable::Cleanup()
{
	m_Dependencies.State_ObjectTable.Cleanup();
	m_Dependencies.System_Debug.Log("[ObjectTableSystem] INFO: Cleanup completed.");
}

// Responsible of updating all the general & specific data of a given object.
void System_ObjectTable::UpdateObjectData(LiveObject& object)
{
	if (object.Address == 0) return;

	using namespace ObjectOffsets;

	auto& reader = m_Dependencies.System_MemoryReader;

	// Update the parent-child-sibling handles.
	object.NextSiblingHandle = reader.Read<uint32_t>(object.Address, NextSiblingHandle);
	object.ChildHandle = reader.Read<uint32_t>(object.Address, ChildHandle);
	object.ParentHandle = reader.Read<uint32_t>(object.Address, ParentHandle);

	// Update the world-space position, and forward/up.
	object.Position = reader.ReadArray<float, 3>(object.Address, CurrentPosition);
	object.Forward = reader.ReadArray<float, 3>(object.Address, Forward);
	object.Up = reader.ReadArray<float, 3>(object.Address, Up);

	object.CurrentRadius = reader.Read<float>(object.Address, CurrentRadius);

	// Update the velocities.
	object.LinearVelocity = reader.ReadArray<float, 3>(object.Address, LinearVelocity);
	object.AngularVelocity = reader.ReadArray<float, 3>(object.Address, AngularVelocity);
	
	object.DamageReceived = reader.Read<float>(object.Address, DamageReceived);

	// Update the specific data.
	switch (object.Profile.Class)
	{
	case ObjectClass::Biped:
	{
		this->UpdateBiped(m_Dependencies.System_MemoryReader, object);
		break;
	}
		
	case ObjectClass::Vehicle:
	{
		this->UpdateVehicle(m_Dependencies.System_MemoryReader, object);
		break;
	}

	case ObjectClass::Weapon:
	{
		this->UpdateWeapon(m_Dependencies.System_MemoryReader, object);
		break;
	}

	case ObjectClass::Equipment:
	{
		this->UpdateEquipment(m_Dependencies.System_MemoryReader, object);
		break;
	}

	case ObjectClass::Projectile:
	{
		this->UpdateProjectiles(m_Dependencies.System_MemoryReader, object);
		break;
	}

	case ObjectClass::Scenery:
	{
		this->UpdateScenery(m_Dependencies.System_MemoryReader, object);
		break;
	}

	// TODO: Device machine...
	// TODO: Device control...

	case ObjectClass::Crate:
	{
		this->UpdateCrate(m_Dependencies.System_MemoryReader, object);
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