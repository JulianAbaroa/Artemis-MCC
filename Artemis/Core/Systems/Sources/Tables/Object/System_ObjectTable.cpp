#include "pch.h"

#include "System_ObjectTable.h"

#include "Core/Types/Sources/MapReader/ResolvedTag.h"

#include "Core/States/Sources/MapReader/Tags/State_MapBipd.h"
#include "Core/States/Sources/MapReader/Tags/State_MapBloc.h"
#include "Core/States/Sources/MapReader/Tags/State_MapColl.h"
#include "Core/States/Sources/MapReader/Tags/State_MapCtrl.h"
#include "Core/States/Sources/MapReader/Tags/State_MapEqip.h"
#include "Core/States/Sources/MapReader/Tags/State_MapHlmt.h"
#include "Core/States/Sources/MapReader/Tags/State_MapLbsp.h"
#include "Core/States/Sources/MapReader/Tags/State_MapMach.h"
#include "Core/States/Sources/MapReader/Tags/State_MapMode.h"
#include "Core/States/Sources/MapReader/Tags/State_MapPhmo.h"
#include "Core/States/Sources/MapReader/Tags/State_MapPlay.h"
#include "Core/States/Sources/MapReader/Tags/State_MapProj.h"
#include "Core/States/Sources/MapReader/Tags/State_MapSbsp.h"
#include "Core/States/Sources/MapReader/Tags/State_MapScen.h"
#include "Core/States/Sources/MapReader/Tags/State_MapScnr.h"
#include "Core/States/Sources/MapReader/Tags/State_MapSldt.h"
#include "Core/States/Sources/MapReader/Tags/State_MapVehi.h"
#include "Core/States/Sources/MapReader/Tags/State_MapWeap.h"
#include "Core/States/Sources/MapReader/Tags/State_MapZone.h"
#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"
#include "Core/States/Sources/Tables/Object/BoneMatrix/State_BoneOffsets.h"
#include "Core/States/Sources/Tables/Object/BoneMatrix/State_BoneMatrices.h"
#include "Core/States/Sources/Tables/Object/DamageSection/State_DamageSections.h"

#include "Core/Systems/Sources/MapReader/System_MapReader.h"
#include "Core/Systems/Other/Memory/Reader/System_MemoryReader.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

void System_ObjectTable::OnObjectCreated(
	uint32_t handle, uint32_t datumIndex)
{
	auto tag = m_Deps.System_MapReader.ResolveHandle(datumIndex);
	if (!tag.IsValid)
	{
		m_Deps.System_Logs.Log("[ObjectTable] WARNING:"
			" Failed to resolve DatumIndex 0x%X for handle 0x%X.", 
			datumIndex, handle);
		return;
	}

	LiveObject object;
	object.Handle = handle;
	object.DatumIndex = datumIndex;
	object.FourCC = tag.FourCC;
	object.TagName = tag.TagName;

	if (object.FourCC == "" || object.TagName == "")
	{
		m_Deps.System_Logs.Log("[ObjectTable] WARNING:"
			" Invalid object created.");
		return;
	}

	ObjectProfile profile;
	this->SetProfile(object, profile);
	object.Profile = profile;

	m_Deps.State_ObjectTable.AddObject(handle, object);
}

void System_ObjectTable::SetProfile(
	LiveObject& object, ObjectProfile& profile)
{
	profile.HasBipd = m_Deps.State_MapBipd.Has(object.TagName);
	profile.HasBloc = m_Deps.State_MapBloc.Has(object.TagName);
	profile.HasColl = m_Deps.State_MapColl.Has(object.TagName);
	profile.HasCtrl = m_Deps.State_MapCtrl.Has(object.TagName);
	profile.HasEqip = m_Deps.State_MapEqip.Has(object.TagName);
	profile.HasHlmt = m_Deps.State_MapHlmt.Has(object.TagName);
	profile.HasLbsp = m_Deps.State_MapLbsp.Has(object.TagName);
	profile.HasMach = m_Deps.State_MapMach.Has(object.TagName);
	profile.HasMode = m_Deps.State_MapMode.Has(object.TagName);
	profile.HasPhmo = m_Deps.State_MapPhmo.Has(object.TagName);
	profile.HasPlay = m_Deps.State_MapPlay.Has(object.TagName);
	profile.HasProj = m_Deps.State_MapProj.Has(object.TagName);
	profile.HasSbsp = m_Deps.State_MapSbsp.Has(object.TagName);
	profile.HasScen = m_Deps.State_MapScen.Has(object.TagName);
	profile.HasScnr = m_Deps.State_MapScnr.Has(object.TagName);
	profile.HasSldt = m_Deps.State_MapSldt.Has(object.TagName);
	profile.HasVehi = m_Deps.State_MapVehi.Has(object.TagName);
	profile.HasWeap = m_Deps.State_MapWeap.Has(object.TagName);
	profile.HasZone = m_Deps.State_MapZone.Has(object.TagName);
}

void System_ObjectTable::OnObjectDestroyed(uint32_t handle)
{
	auto deletedObject = 
		m_Deps.State_ObjectTable.RemoveObject(handle);

	if (!deletedObject.has_value())
	{
		m_Deps.System_Logs.Log("[ObjectTable] WARNING:"
			" OnObjectDestroyed called for unknown handle 0x%X.", handle);
	}
}

void System_ObjectTable::UpdateObjectTable()
{
	uintptr_t tableBase = m_Deps.State_ObjectTable.GetBase();
	if (tableBase == 0) return;

	m_Deps.State_BoneMatrices.Clear();
	m_Deps.State_DamageSections.Clear();

	m_Deps.State_ObjectTable.UpdateObjects(
		[&](uint32_t handle, LiveObject& object) {
		uint32_t index = handle & 0xFFFF;
		uintptr_t offset = (uintptr_t)index * 0x18;
	
		uintptr_t entryAddr = tableBase + offset;

		if (entryAddr == 0) return;
		
		uint16_t tableSalt = *(uint16_t*)(entryAddr);
		uint16_t expectedSalt = (uint16_t)(handle >> 16);
		uintptr_t entityPtr = *(uintptr_t*)(entryAddr + 0x10);
		
		if (entityPtr == 0 || tableSalt != expectedSalt)
		{
			object.Address = 0;
		}

		if (object.Address != entityPtr) 
		{
			object.Address = entityPtr;
		}
		
		ObjectClass objectClass = 
			*(ObjectClass*)(entryAddr + 0x04);
		
		if (object.Profile.Class == ObjectClass::Invalid &&
			objectClass != ObjectClass::Invalid)
		{
			object.Profile.Class = objectClass;
		}
		
		this->UpdateObjectData(object);
	});

	m_Deps.State_ObjectTable.Publish();
}

void System_ObjectTable::UpdateObjectData(LiveObject& object)
{
	if (object.Address == 0) return;

	using namespace ObjectOffsets;

	auto& reader = m_Deps.System_MemoryReader;

	object.NextSiblingHandle = reader.Read<uint32_t>(object.Address, NextSiblingHandle);
	object.ChildHandle = reader.Read<uint32_t>(object.Address, ChildHandle);
	object.ParentHandle = reader.Read<uint32_t>(object.Address, ParentHandle);

	object.Position = reader.ReadArray<float, 3>(object.Address, CurrentPosition);
	object.Forward = reader.ReadArray<float, 3>(object.Address, Forward);
	object.Up = reader.ReadArray<float, 3>(object.Address, Up);
	object.LinearVelocity = reader.ReadArray<float, 3>(object.Address, LinearVelocity);
	object.AngularVelocity = reader.ReadArray<float, 3>(object.Address, AngularVelocity);
	
	object.CurrentRadius = reader.Read<float>(object.Address, CurrentRadius);
	object.DamageReceived = reader.Read<float>(object.Address, DamageReceived);

	object.HlmtVariant = reader.Read<uint8_t>(object.Address, HlmtVariant);

	this->ReadBoneMatrixTable(reader, object);
	this->ReadDamageSectionTable(reader, object);

	switch (object.Profile.Class)
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

void System_ObjectTable::ReadBoneMatrixTable(
	System_MemoryReader& reader, LiveObject& object)
{
	const std::optional<BonesHeader> header =
		m_Deps.State_BoneOffsets.Get(object.Handle);

	if (!header) return;

	const uintptr_t offset = header->Offset;
	const size_t nodeCount = header->NodeCount;

	if (nodeCount == 0) return;

	BoneMatrixTable table;
	table.BaseAddress = object.Address + offset;
	table.Matrices.resize(nodeCount);

	const size_t bytes = nodeCount * sizeof(BoneMatrix);
	reader.ReadRaw(table.BaseAddress, table.Matrices.data(), bytes);

	m_Deps.State_BoneMatrices.Set(object.Handle, std::move(table));
}

void System_ObjectTable::ReadDamageSectionTable(
	System_MemoryReader& reader, LiveObject& object)
{
	using namespace ObjectOffsets;

	const uint16_t regionsSize = reader.Read<uint16_t>(object.Address, DamageRegionsSize);
	const uint16_t regionsOffsetRaw = reader.Read<uint16_t>(object.Address, DamageRegionsOffset);

	if (regionsOffsetRaw == 0xFFFF) return;
	const uint16_t count = regionsSize / m_kDamageSectionStride;
	if (count == 0 || count >= 256) return;

	DamageSectionTable table;
	table.BaseAddress = object.Address + regionsOffsetRaw;
	table.Sections.resize(count);

	std::vector<DamageSection> raw(count);
	reader.ReadRaw(table.BaseAddress, raw.data(), count * sizeof(DamageSection));

	for (uint16_t section = 0; section < count; ++section)
	{
		table.Sections[section].DamageLevelMask = 
			raw[section].DamageLevelMask;

		table.Sections[section].Vitality = 
			raw[section].Vitality;
	}

	m_Deps.State_DamageSections.Set(object.Handle, std::move(table));
}

void System_ObjectTable::UpdateBiped(
	System_MemoryReader& reader, LiveObject& object)
{
	namespace Biped = ObjectOffsets::Biped;

	BipedObject biped{};

	biped.IsCrouched = reader.Read<uint8_t>(object.Address, Biped::VerticalState);
	biped.IsGrounded = reader.Read<uint8_t>(object.Address, Biped::IsGrounded);
	biped.IsAbilityActive = reader.Read<uint8_t>(object.Address, Biped::IsAbilityActive);
	biped.ZoomLevel = reader.Read<ZoomLevel>(object.Address, Biped::ZoomLevel);

	biped.MovementDirection = reader.ReadArray<float, 2>(object.Address, Biped::MovementDirection);
	biped.SurfaceNormal = reader.ReadArray<float, 3>(object.Address, Biped::SurfaceNormal);
	biped.GroundObjectHandle = reader.Read<uint32_t>(object.Address, Biped::GroundObjectHandle);
	biped.MaterialIndex = reader.Read<uint16_t>(object.Address, Biped::MaterialIndex);

	biped.DamagerBipedHandle = reader.Read<uint32_t>(object.Address, Biped::DamagerBipedHandle);
	biped.DamagerPlayerHandle = reader.Read<uint32_t>(object.Address, Biped::DamagerPlayerHandle);

	object.SpecificObject = biped;
}

void System_ObjectTable::UpdateVehicle(
	System_MemoryReader& reader, LiveObject& object)
{
	namespace Vehicle = ObjectOffsets::Vehicle;
	namespace Helpers = VehicleObjectHelpers;

	VehicleObject vehicle{};

	vehicle.Base = object.Address;
	vehicle.Type = Helpers::ResolveVehicleType(object.TagName);
	vehicle.SeatLayout = Helpers::GetSeatLayout(vehicle.Type);

	if (TagName::Vehicle::HasBoost(object.TagName))
	{
		vehicle.BoostThrottle = reader.Read<float>(object.Address, Vehicle::BoostThrottle);
		vehicle.BoostEnergy = reader.Read<float>(object.Address, Vehicle::BoostEnergy);
		vehicle.BoostCooldown = reader.Read<float>(object.Address, Vehicle::BoostCooldown);
	}

	object.SpecificObject = vehicle;
}

void System_ObjectTable::UpdateWeapon(
	System_MemoryReader& reader, LiveObject& object)
{
	namespace Weapon = ObjectOffsets::Weapon;

	WeaponObject weapon{};

	weapon.TotalHeat = reader.Read<float>(object.Address, Weapon::TotalHeat);
	weapon.TotalEnergy = 1.0f - reader.Read<float>(object.Address, Weapon::TotalEnergy);
	weapon.TotalAmmo = reader.Read<uint16_t>(object.Address, Weapon::TotalAmmo);
	weapon.CurrentAmmo = reader.Read<uint16_t>(object.Address, Weapon::CurrentAmmo);

	weapon.IsFiring = reader.Read<uint8_t>(object.Address, Weapon::IsFiring);
	weapon.IsReloading = reader.Read<uint8_t>(object.Address, Weapon::IsReloading);
	weapon.ChargeProgress = reader.Read<uint8_t>(object.Address, Weapon::ChargeProgress);
	weapon.ActionState = reader.Read<ActionState>(object.Address, Weapon::ActionState);

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

void System_ObjectTable::UpdateEquipment(
	System_MemoryReader& reader, LiveObject& object)
{
	namespace Equipment = ObjectOffsets::Equipment;

	EquipmentObject equipment{};

	equipment.TotalEnergy = reader.Read<float>(object.Address, Equipment::TotalEnergy);

	object.SpecificObject = equipment;
}

void System_ObjectTable::UpdateProjectiles(
	System_MemoryReader& reader, LiveObject& object)
{
	namespace Projectile = ObjectOffsets::Projectile;

	ProjectileObject projectile{};

	projectile.ProjectileFlags = reader.Read<ProjectileFlags>(object.Address, Projectile::RuntimeFlags);
	projectile.OwnerBipedHandle = reader.Read<uint32_t>(object.Address, Projectile::OwnerBipedHandle);
	projectile.OwnerWeaponHandle = reader.Read<uint32_t>(object.Address, Projectile::OwnerWeaponHandle);

	object.SpecificObject = projectile;
}

void System_ObjectTable::UpdateCrate(
	System_MemoryReader& reader, LiveObject& object)
{
	namespace Crate = ObjectOffsets::Crate;
	namespace Helpers = CrateObjectHelpers;

	CrateObject crate{};

	crate.Base = object.Address;
	crate.Type = Helpers::ResolveCrateType(object.TagName);

	if (auto offsets = Helpers::ResolveZoneOffsets(crate.Type))
	{
		ZoneInfo zone{};

		zone.Shape.Radius = reader.Read<float>(object.Address, offsets->Radius);
		zone.Shape.Length = reader.Read<float>(object.Address, offsets->Length);
		zone.Shape.Top = reader.Read<float>(object.Address, offsets->Top);
		zone.Shape.Bottom = reader.Read<float>(object.Address, offsets->Bottom);
		zone.Shape.ShapeType = reader.Read<ShapeType>(object.Address, offsets->ShapeType);
		zone.Team = reader.Read<Team>(object.Address, offsets->Team);

		crate.Zone = zone;
	}

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

	else if (crate.Type == CrateType::Lift)
	{
		LiftData lift{};

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

void System_ObjectTable::UpdateScenery(
	System_MemoryReader& reader, LiveObject& object)
{
	namespace Scenery = ObjectOffsets::Scenery;
	namespace Helpers = SceneryObjectHelpers;

	SceneryObject scenery{};
	scenery.Base = object.Address;
	scenery.Type = Helpers::ResolveSceneryType(object.TagName);

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

	object.SpecificObject = scenery;
}

void System_ObjectTable::Cleanup()
{
	m_Deps.State_ObjectTable.Cleanup();
	m_Deps.State_BoneOffsets.Cleanup();

	m_Deps.System_Logs.Log("[ObjectTable] INFO:"
		" Cleanup completed.");
}