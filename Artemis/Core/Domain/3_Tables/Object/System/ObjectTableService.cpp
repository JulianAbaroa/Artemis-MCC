module Tables.Object.System;

import Common.Math.Type;
import Common.ZoneShape.Type;
import Common.Tag.Name;
import Common.Team.Type;

namespace
{
	namespace Offset = Tables::Object::Type::Offset;
	namespace TagName = Common::Tag::Name;

	using Vec2 = Common::Math::Type::Vec2;
	using Vec3 = Common::Math::Type::Vec3;
	using ZoneShape = Common::ZoneShape::Type::ZoneShape;
	using ShapeKind = Common::ZoneShape::Type::Kind;
	using Team = Common::Team::Type::Team;
	using Class = Tables::Object::Type::Class::Class;
	using BonesHeader = Tables::Object::Type::BoneMatrix::BonesHeader;
	using BipedObject = Tables::Object::Type::Biped::Biped;
	using CrateObject = Tables::Object::Type::Crate::Crate;
	using EquipmentObject = Tables::Object::Type::Equipment::Equipment;
	using ProjectileObject = Tables::Object::Type::Projectile::Projectile;
	using SceneryObject = Tables::Object::Type::Scenery::Scenery;
	using VehicleObject = Tables::Object::Type::Vehicle::Vehicle;
	using WeaponObject = Tables::Object::Type::Weapon::Weapon;
	using ProjectileFlags = Tables::Object::Type::Projectile::Flags;
	using Zone = Tables::Object::Type::Crate::Zone::Zone;
	using Teleport = Tables::Object::Type::Crate::Teleport::Teleport;
	using Allowed = Tables::Object::Type::Crate::Teleport::Allowed;
	using CrateKind = Tables::Object::Type::Crate::Kind;
	using ActionState = Tables::Object::Type::Weapon::ActionState;
	using LiftAngle = Tables::Object::Type::Crate::Lift::Angle;
	using LiftForce = Tables::Object::Type::Crate::Lift::Force;
	using Lift = Tables::Object::Type::Crate::Lift::Lift;
	using ShieldKind = Tables::Object::Type::Crate::Shield::Kind;
	using ZoomLevel = Tables::Object::Type::Biped::ZoomLevel;
	using Boundary = Tables::Object::Type::Scenery::Boundary::Boundary;
	using SceneryKind = Tables::Object::Type::Scenery::Kind;
	using Shield = Tables::Object::Type::Crate::Shield::Shield;
	using SpawnData = Tables::Object::Type::Scenery::Spawn::Spawn;
}

namespace Tables::Object::System
{
	auto ObjectTableService::OnObjectCreated(std::uint32_t handle, std::uint32_t datumIndex) -> void
	{
		auto tag = m_TagResolverService.ResolveHandle(datumIndex);
		if (!tag.IsValid)
		{
			m_LogsService.Message("[ObjectTableService] WARNING:"
				" Failed to resolve DatumIndex 0x{:X} for handle 0x{:X}.",
				datumIndex, handle);
			return;
		}

		AliveObject object;
		object.Handle = handle;
		object.DatumIndex = datumIndex;
		object.FourCC = tag.FourCC;
		object.TagName = tag.TagName;

		if (object.FourCC == "" || object.TagName == "")
		{
			m_LogsService.Message("[ObjectTableService] WARNING:"
				" Invalid object created.");
			return;
		}

		Profile profile;
		this->SetProfile(object, profile);
		object.Profile = profile;

		m_ObjectStore.AddObject(handle, object);
	}

	auto ObjectTableService::SetProfile(AliveObject& object, Profile& profile) -> void
	{
		profile.HasBipd = m_TagResolverService.HasBipd(object.TagName);
		profile.HasBloc = m_TagResolverService.HasBloc(object.TagName);
		profile.HasColl = m_TagResolverService.HasColl(object.TagName);
		profile.HasCtrl = m_TagResolverService.HasCtrl(object.TagName);
		profile.HasEqip = m_TagResolverService.HasEqip(object.TagName);
		profile.HasHlmt = m_TagResolverService.HasHlmt(object.TagName);
		profile.HasMach = m_TagResolverService.HasMach(object.TagName);
		profile.HasMode = m_TagResolverService.HasMode(object.TagName);
		profile.HasPhmo = m_TagResolverService.HasPhmo(object.TagName);
		profile.HasProj = m_TagResolverService.HasProj(object.TagName);
		profile.HasScen = m_TagResolverService.HasScen(object.TagName);
		profile.HasScnr = m_TagResolverService.HasScnr(object.TagName);
		profile.HasVehi = m_TagResolverService.HasVehi(object.TagName);
		profile.HasWeap = m_TagResolverService.HasWeap(object.TagName);
	}

	auto ObjectTableService::OnObjectDestroyed(std::uint32_t handle) -> void
	{
		auto deletedObject = m_ObjectStore.RemoveObject(handle);

		if (!deletedObject.has_value())
		{
			m_LogsService.Message("[ObjectTableService] WARNING:"
				" OnObjectDestroyed called for unknown handle 0x{:X}.", handle);
		}
	}

	auto ObjectTableService::UpdateObjectTable() -> void
	{
		std::uintptr_t tableBase = m_ObjectStore.GetBase();
		if (tableBase == 0) return;

		m_BoneMatricesStore.Clear();
		m_DamageSectionsStore.Clear();

		m_ObjectStore.UpdateObjects(
			[&](std::uint32_t handle, AliveObject& object) {
				std::uint32_t index = handle & 0xFFFF;
				std::uintptr_t offset = (std::uintptr_t)index * 0x18;

				std::uintptr_t entryAddr = tableBase + offset;

				if (entryAddr == 0) return;

				std::uint16_t tableSalt = *(std::uint16_t*)(entryAddr);
				std::uint16_t expectedSalt = (std::uint16_t)(handle >> 16);
				std::uintptr_t entityPtr = *(std::uintptr_t*)(entryAddr + 0x10);

				const bool isAlive = (entityPtr != 0 && tableSalt == expectedSalt);
				object.Address = isAlive ? entityPtr : 0;

				if (!isAlive) return;

				Class objectClass = *(Class*)(entryAddr + 0x04);

				if (object.Profile.Class == Class::Invalid &&
					objectClass != Class::Invalid)
				{
					object.Profile.Class = objectClass;
				}

				this->UpdateObjectData(object);
			});

		m_ObjectStore.Publish();
	}

	auto ObjectTableService::UpdateObjectData(AliveObject& object) -> void
	{
		if (object.Address == 0) return;

		auto& reader = m_MemoryReaderService;

		object.NextSiblingHandle = reader.Read<std::uint32_t>(object.Address, Offset::NextSiblingHandle);
		object.ChildHandle = reader.Read<std::uint32_t>(object.Address, Offset::ChildHandle);
		object.ParentHandle = reader.Read<std::uint32_t>(object.Address, Offset::ParentHandle);

		object.Position = reader.Read<Vec3>(object.Address, Offset::CurrentPosition);
		object.Forward = reader.Read<Vec3>(object.Address, Offset::Forward);
		object.Up = reader.Read<Vec3>(object.Address, Offset::Up);
		object.LinearVelocity = reader.Read<Vec3>(object.Address, Offset::LinearVelocity);
		object.AngularVelocity = reader.Read<Vec3>(object.Address, Offset::AngularVelocity);

		object.CurrentRadius = reader.Read<float>(object.Address, Offset::CurrentRadius);
		object.DamageReceived = reader.Read<float>(object.Address, Offset::DamageReceived);

		object.HlmtVariant = reader.Read<std::uint8_t>(object.Address, Offset::HlmtVariant);

		this->ReadBoneMatrixTable(reader, object);
		this->ReadDamageSectionTable(reader, object);

		switch (object.Profile.Class)
		{
		case Class::Biped:
		{
			this->UpdateBiped(reader, object);
			break;
		}

		case Class::Vehicle:
		{
			this->UpdateVehicle(reader, object);
			break;
		}

		case Class::Weapon:
		{
			this->UpdateWeapon(reader, object);
			break;
		}

		case Class::Equipment:
		{
			this->UpdateEquipment(reader, object);
			break;
		}

		case Class::Projectile:
		{
			this->UpdateProjectiles(reader, object);
			break;
		}

		case Class::Scenery:
		{
			this->UpdateScenery(reader, object);
			break;
		}

		case Class::Crate:
		{
			this->UpdateCrate(reader, object);
			break;
		}

		default:
			object.Specific = std::monostate{};
			break;
		}
	}

	auto ObjectTableService::ReadBoneMatrixTable(MemoryReaderService& reader, AliveObject& object) -> void
	{
		const std::optional<BonesHeader> header =
			m_BoneOffsetsStore.Get(object.Handle);

		if (!header) return;

		const std::uintptr_t offset = header->Offset;
		const std::size_t nodeCount = header->NodeCount;

		if (nodeCount == 0) return;

		BoneMatrixTable table;
		table.BaseAddress = object.Address + offset;
		table.Matrices.resize(nodeCount);

		const std::size_t bytes = nodeCount * sizeof(BoneMatrix);
		if (!reader.ReadRaw(table.BaseAddress, table.Matrices.data(), bytes)) return;

		m_BoneMatricesStore.Set(object.Handle, std::move(table));
	}

	auto ObjectTableService::ReadDamageSectionTable(MemoryReaderService& reader, AliveObject& object) -> void
	{
		const std::uint16_t regionsSize = reader.Read<std::uint16_t>(object.Address, Offset::DamageRegionsSize);
		const std::uint16_t regionsOffsetRaw = reader.Read<std::uint16_t>(object.Address, Offset::DamageRegionsOffset);

		if (regionsOffsetRaw == 0xFFFF) return;
		const std::uint16_t count = regionsSize / m_kDamageSectionStride;
		if (count == 0 || count >= 256) return;

		DamageSectionTable table;
		table.BaseAddress = object.Address + regionsOffsetRaw;
		table.Sections.resize(count);

		std::vector<DamageSection> raw(count);
		if (reader.ReadRaw(table.BaseAddress, raw.data(), count * sizeof(DamageSection))) return;

		for (std::uint16_t section = 0; section < count; ++section)
		{
			table.Sections[section].DamageLevelMask =
				raw[section].DamageLevelMask;

			table.Sections[section].Vitality =
				raw[section].Vitality;
		}

		m_DamageSectionsStore.Set(object.Handle, std::move(table));
	}

	auto ObjectTableService::UpdateBiped(MemoryReaderService& reader, AliveObject& object) -> void
	{
		BipedObject biped{};

		biped.IsCrouched = reader.Read<std::uint8_t>(object.Address, Offset::Biped::VerticalState);
		biped.IsGrounded = reader.Read<std::uint8_t>(object.Address, Offset::Biped::IsGrounded);
		biped.IsAbilityActive = reader.Read<std::uint8_t>(object.Address, Offset::Biped::IsAbilityActive);
		biped.ZoomLevel = reader.Read<ZoomLevel>(object.Address, Offset::Biped::ZoomLevel);

		biped.MovementDirection = reader.Read<Vec2>(object.Address, Offset::Biped::MovementDirection);
		biped.SurfaceNormal = reader.Read<Vec3>(object.Address, Offset::Biped::SurfaceNormal);
		biped.GroundObjectHandle = reader.Read<std::uint32_t>(object.Address, Offset::Biped::GroundObjectHandle);
		biped.MaterialIndex = reader.Read<std::uint16_t>(object.Address, Offset::Biped::MaterialIndex);

		biped.DamagerBipedHandle = reader.Read<std::uint32_t>(object.Address, Offset::Biped::DamagerBipedHandle);
		biped.DamagerPlayerHandle = reader.Read<std::uint32_t>(object.Address, Offset::Biped::DamagerPlayerHandle);

		object.Specific = biped;
	}

	auto ObjectTableService::UpdateVehicle(MemoryReaderService& reader, AliveObject& object) -> void
	{
		VehicleObject vehicle{};

		vehicle.Base = object.Address;
		vehicle.Kind = Tables::Object::Type::Vehicle::ResolveVehicleType(object.TagName);
		vehicle.SeatLayout = Tables::Object::Type::Vehicle::GetSeatLayout(vehicle.Kind);

		if (TagName::Vehicle::HasBoost(object.TagName))
		{
			vehicle.BoostThrottle = reader.Read<float>(object.Address, Offset::Vehicle::BoostThrottle);
			vehicle.BoostEnergy = reader.Read<float>(object.Address, Offset::Vehicle::BoostEnergy);
			vehicle.BoostCooldown = reader.Read<float>(object.Address, Offset::Vehicle::BoostCooldown);
		}

		object.Specific = vehicle;
	}

	auto ObjectTableService::UpdateWeapon(MemoryReaderService& reader, AliveObject& object) -> void
	{
		WeaponObject weapon{};

		weapon.TotalHeat = reader.Read<float>(object.Address, Offset::Weapon::TotalHeat);
		weapon.TotalEnergy = 1.0f - reader.Read<float>(object.Address, Offset::Weapon::TotalEnergy);
		weapon.TotalAmmo = reader.Read<std::uint16_t>(object.Address, Offset::Weapon::TotalAmmo);
		weapon.CurrentAmmo = reader.Read<std::uint16_t>(object.Address, Offset::Weapon::CurrentAmmo);

		weapon.IsFiring = reader.Read<std::uint8_t>(object.Address, Offset::Weapon::IsFiring);
		weapon.IsReloading = reader.Read<std::uint8_t>(object.Address, Offset::Weapon::IsReloading);
		weapon.ChargeProgress = reader.Read<std::uint8_t>(object.Address, Offset::Weapon::ChargeProgress);
		weapon.ActionState = reader.Read<ActionState>(object.Address, Offset::Weapon::ActionState);

		if (object.TagName == TagName::Objective::Flag)
		{
			weapon.Team = reader.Read<Team>(object.Address, Offset::Weapon::Flag::Team);
		}
		else if (object.TagName == TagName::Objective::Bomb)
		{
			weapon.Team = reader.Read<Team>(object.Address, Offset::Weapon::Bomb::Team);
		}

		object.Specific = weapon;
	}

	auto ObjectTableService::UpdateEquipment(MemoryReaderService& reader, AliveObject& object) -> void
	{
		EquipmentObject equipment{};

		equipment.Energy = reader.Read<float>(object.Address, Offset::Equipment::TotalEnergy);

		object.Specific = equipment;
	}

	auto ObjectTableService::UpdateProjectiles(MemoryReaderService& reader, AliveObject& object) -> void
	{
		ProjectileObject projectile{};

		projectile.Flags = reader.Read<ProjectileFlags>(object.Address, Offset::Projectile::RuntimeFlags);
		projectile.OwnerBipedHandle = reader.Read<std::uint32_t>(object.Address, Offset::Projectile::OwnerBipedHandle);
		projectile.OwnerWeaponHandle = reader.Read<std::uint32_t>(object.Address, Offset::Projectile::OwnerWeaponHandle);

		object.Specific = projectile;
	}

	auto ObjectTableService::UpdateCrate(MemoryReaderService& reader, AliveObject& object) -> void
	{
		CrateObject crate{};

		crate.Base = object.Address;
		crate.Kind = Tables::Object::Type::Crate::ResolveCrateType(object.TagName);

		if (auto offsets = Tables::Object::Type::Crate::ResolveZoneOffsets(crate.Kind))
		{
			Zone zone{};

			zone.Shape.Radius = reader.Read<float>(object.Address, offsets->Radius);
			zone.Shape.Length = reader.Read<float>(object.Address, offsets->Length);
			zone.Shape.Top = reader.Read<float>(object.Address, offsets->Top);
			zone.Shape.Bottom = reader.Read<float>(object.Address, offsets->Bottom);
			zone.Shape.Kind = reader.Read<ShapeKind>(object.Address, offsets->ZoneType);
			zone.Team = reader.Read<Team>(object.Address, offsets->Team);

			crate.Zone = zone;
		}

		else if (crate.Kind == CrateKind::TeleportSender ||
			crate.Kind == CrateKind::TeleportReceiver ||
			crate.Kind == CrateKind::TeleportTwoWay)
		{
			Teleport teleport{};

			teleport.ZoneShape.Radius = reader.Read<float>(object.Address, Offset::Crate::Teleport::Radius);
			teleport.ZoneShape.Length = reader.Read<float>(object.Address, Offset::Crate::Teleport::Length);
			teleport.ZoneShape.Top = reader.Read<float>(object.Address, Offset::Crate::Teleport::Top);
			teleport.ZoneShape.Bottom = reader.Read<float>(object.Address, Offset::Crate::Teleport::Bottom);
			teleport.ZoneShape.Kind = reader.Read<ShapeKind>(object.Address, Offset::Crate::Teleport::ShapeKind);
			teleport.Channel = reader.Read<std::uint8_t>(object.Address, Offset::Crate::Teleport::Channel);
			teleport.Allowed = reader.Read<Allowed>(object.Address, Offset::Crate::Teleport::Allowed);

			crate.Teleport = teleport;
		}

		else if (crate.Kind == CrateKind::Lift)
		{
			Lift lift{};

			if (TagName::Lift::IsCurved(object.TagName))
			{
				lift.Angle = LiftAngle::Curved;
			}
			else if (TagName::Lift::IsVertical(object.TagName))
			{
				lift.Angle = LiftAngle::Vertical;
			}
			else if (TagName::Lift::IsRedirected(object.TagName))
			{
				lift.Angle = LiftAngle::Redirected;
			}

			if (TagName::Lift::IsDefault(object.TagName))
			{
				lift.Force = LiftForce::Default;
			}
			else if (TagName::Lift::IsLight(object.TagName))
			{
				lift.Force = LiftForce::Light;
			}
			else if (TagName::Lift::IsHeavy(object.TagName))
			{
				lift.Force = LiftForce::Heavy;
			}
			else if (TagName::Lift::IsVehicle(object.TagName))
			{
				lift.Force = LiftForce::Vehicle;
			}

			crate.Lift = lift;
		}

		else if (crate.Kind == CrateKind::Shield)
		{
			Shield shield{};

			if (TagName::Shield::IsOneWay(object.TagName))
			{
				shield.Kind = ShieldKind::OneWay;
			}
			else if (TagName::Shield::IsTwoWay(object.TagName))
			{
				shield.Kind = ShieldKind::TwoWay;

				if (TagName::Shield::IsShieldDoor(object.TagName))
				{
					shield.IsShieldDoor = true;
				}
			}
			else if (TagName::Shield::IsBlocker(object.TagName))
			{
				shield.Kind = ShieldKind::Blocker;
			}

			crate.Shield = shield;
		}

		object.Specific = crate;
	}

	auto ObjectTableService::UpdateScenery(MemoryReaderService& reader, AliveObject& object) -> void
	{
		SceneryObject scenery{};
		scenery.Base = object.Address;
		scenery.Kind = Tables::Object::Type::Scenery::ResolveSceneryType(object.TagName);

		if (Tables::Object::Type::Scenery::IsSpawnPoint(scenery.Kind))
		{
			SpawnData spawn{};

			if (scenery.Kind == SceneryKind::InvisibleRespawnPoint)
			{
				spawn.Team = Team::Neutral;
			}
			else
			{
				spawn.Team = reader.Read<Team>(
					object.Address, Offset::Scenery::SpawnPoint::Team);
			}

			scenery.Spawn = spawn;
		}

		else if (Tables::Object::Type::Scenery::IsBoundary(scenery.Kind))
		{
			Boundary boundary{};

			boundary.Shape.Radius = reader.Read<float>(object.Address, Offset::Scenery::Boundary::Radius);
			boundary.Shape.Length = reader.Read<float>(object.Address, Offset::Scenery::Boundary::Length);
			boundary.Shape.Top = reader.Read<float>(object.Address, Offset::Scenery::Boundary::Top);
			boundary.Shape.Bottom = reader.Read<float>(object.Address, Offset::Scenery::Boundary::Bottom);
			boundary.Shape.Kind = reader.Read<ShapeKind>(object.Address, Offset::Scenery::Boundary::ShapeKind);
			boundary.Team = reader.Read<Team>(object.Address, Offset::Scenery::Boundary::Team);

			scenery.Boundary = boundary;
		}

		object.Specific = scenery;
	}

	auto ObjectTableService::Cleanup() -> void
	{
		m_ObjectStore.Cleanup();
		m_BoneOffsetsStore.Cleanup();

		m_LogsService.Message("[ObjectTableService] INFO: Cleanup completed.");
	}
}