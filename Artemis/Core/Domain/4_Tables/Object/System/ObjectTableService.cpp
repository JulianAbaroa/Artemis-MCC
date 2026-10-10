module Tables.Object.System;

import Common.Math.Type;
import Common.ZoneShape.Type;
import Common.Tag.Type;
import Common.Team.Type;
import Resolved.Definitions.Type;
import Template.Object.Type;

namespace
{
	namespace Offset = Tables::Object::Type::Offset;
	namespace TagName = Common::Tag::Type;

	using Vec2 = Common::Math::Type::Vec2;
	using Vec3 = Common::Math::Type::Vec3;
	using ZoneShape = Common::ZoneShape::Type::ZoneShape;
	using ShapeKind = Common::ZoneShape::Type::Kind;
	using Team = Common::Team::Type::Team;
	using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
	using RawObject = Template::Object::Type::Raw::RawObject;
	using Snapshot = Template::Object::Type::Raw::Snapshot;
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
	using WeaponAction = Tables::Object::Type::Weapon::WeaponAction;
	using ChargeState = Tables::Object::Type::Weapon::ChargeState;
	using LiftAngle = Tables::Object::Type::Crate::Lift::Angle;
	using LiftForce = Tables::Object::Type::Crate::Lift::Force;
	using Lift = Tables::Object::Type::Crate::Lift::Lift;
	using ShieldKind = Tables::Object::Type::Crate::Shield::Kind;
	using ZoomLevel = Tables::Object::Type::Biped::ZoomLevel;
	using VerticalInput = Tables::Object::Type::Biped::VerticalInput;
	using Boundary = Tables::Object::Type::Scenery::Boundary::Boundary;
	using SceneryKind = Tables::Object::Type::Scenery::Kind;
	using Shield = Tables::Object::Type::Crate::Shield::Shield;
	using SpawnData = Tables::Object::Type::Scenery::Spawn::Spawn;

	using Tables::Object::Type::Constant::k_DamageSectionStride;
	using Tables::Object::Type::Constant::k_RegionBlockMaxSize;

	constexpr std::uint8_t k_TriggerHeldBit{ 0x02 };
	constexpr std::uint8_t k_ZoomedBit{ 0x80 };
	constexpr std::uint8_t k_ActionMask{ 0x60 };
}

namespace Tables::Object::System
{
	auto ObjectTableService::AddObject(std::uint32_t handle, std::uint32_t datumIndex, ObjectKind objectKind) -> void
	{
		auto tag = m_TagResolverService.ResolveHandle(datumIndex);
		if (!tag.IsValid) return;

		AliveObject object;
		object.Handle = handle;
		object.DatumIndex = datumIndex;
		object.FourCC = tag.FourCC;
		object.TagName = tag.TagName;

		if (object.FourCC == "" || object.TagName == "") return;

		Profile profile;
		this->SetProfile(object, profile);
		profile.ObjectKind = objectKind;
		object.Profile = profile;

		m_ObjectStore.AddObject(handle, object);
	}

	auto ObjectTableService::SetProfile(AliveObject& object, Profile& profile) -> void
	{
		profile.HasColl = m_TagResolverService.HasColl(object.TagName);
		profile.HasHlmt = m_TagResolverService.HasHlmt(object.TagName);
		profile.HasMode = m_TagResolverService.HasMode(object.TagName);
	}

	auto ObjectTableService::DiscoverObjects(const Snapshot& snapshot) -> void
	{
		for (const auto& [handle, datum] : snapshot.RawObjects)
		{
			if (m_ObjectStore.HasObject(handle)) continue;

			std::uint32_t datumIndex{ datum.Read<std::uint32_t>(Offset::DatumIndex) };

			this->AddObject(handle, datumIndex, datum.Kind);
		}
	}

	auto ObjectTableService::UpdateObjectTable() -> void
	{
		auto snapshot = m_TemplateStore.Acquire();
		if (!snapshot) return;

		m_BoneMatricesStore.Clear();
		m_DamageSectionsStore.Clear();

		this->DiscoverObjects(*snapshot);

		std::vector<std::uint32_t> deadHandles{};

		m_ObjectStore.UpdateObjects(
			[&](std::uint32_t handle, AliveObject& object) {
				auto it = snapshot->RawObjects.find(handle);
				if (it == snapshot->RawObjects.end())
				{
					deadHandles.push_back(handle);
					return;
				}

				object.Address = it->second.Address;

				this->UpdateObjectData(it->second, object);
			});

		for (std::uint32_t handle : deadHandles)
		{
			m_ObjectStore.RemoveObject(handle);
		}

		m_ObjectStore.Publish();
	}

	auto ObjectTableService::UpdateObjectData(const RawObject& datum, AliveObject& object) -> void
	{
		object.NextSiblingHandle = datum.Read<std::uint32_t>(Offset::NextSiblingHandle);
		object.ChildHandle = datum.Read<std::uint32_t>(Offset::ChildHandle);
		object.ParentHandle = datum.Read<std::uint32_t>(Offset::ParentHandle);

		object.Position = datum.Read<Vec3>(Offset::BoundingCenter);
		object.Origin = datum.Read<Vec3>(Offset::Origin);
		object.Forward = datum.Read<Vec3>(Offset::Forward);
		object.Up = datum.Read<Vec3>(Offset::Up);
		object.LinearVelocity = datum.Read<Vec3>(Offset::LinearVelocity);
		object.AngularVelocity = datum.Read<Vec3>(Offset::AngularVelocity);

		object.BoundingRadius = datum.Read<float>(Offset::BoundingRadius);
		object.DamageReceived = datum.Read<float>(Offset::DamageReceived);

		object.HlmtVariant = datum.Read<std::uint8_t>(Offset::HlmtVariant);

		this->ReadBoneMatrixTable(datum, object);
		this->ReadDamageSectionTable(datum, object);

		switch (object.Profile.ObjectKind)
		{
		case ObjectKind::Biped:
		{
			this->UpdateBiped(datum, object);
			break;
		}

		case ObjectKind::Vehicle:
		{
			this->UpdateVehicle(datum, object);
			break;
		}

		case ObjectKind::Weapon:
		{
			this->UpdateWeapon(datum, object);
			break;
		}

		case ObjectKind::Equipment:
		{
			this->UpdateEquipment(datum, object);
			break;
		}

		case ObjectKind::Projectile:
		{
			this->UpdateProjectiles(datum, object);
			break;
		}

		case ObjectKind::Scenery:
		{
			this->UpdateScenery(datum, object);
			break;
		}

		case ObjectKind::Crate:
		{
			this->UpdateCrate(datum, object);
			break;
		}

		default:
			object.Specific = std::monostate{};
			break;
		}
	}

	auto ObjectTableService::ReadBoneMatrixTable(const RawObject& datum, AliveObject& object) -> void
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
		if (!datum.ReadRaw(offset, table.Matrices.data(), bytes)) return;

		m_BoneMatricesStore.Set(object.Handle, std::move(table));
	}

	auto ObjectTableService::ReadDamageSectionTable(const RawObject& datum, AliveObject& object) -> void
	{
		DamageSectionTable table{};

		this->ReadDamageSections(datum, object, table);
		this->ReadRegionBlock(datum, object, table);

		if (table.Sections.empty() && table.Regions.Bytes.empty()) return;

		m_DamageSectionsStore.Set(object.Handle, std::move(table));
	}

	auto ObjectTableService::ReadDamageSections(const RawObject& datum, const AliveObject& object, DamageSectionTable& table) -> void
	{
		const std::uint16_t regionsSize = datum.Read<std::uint16_t>(Offset::DamageRegionsSize);
		const std::uint16_t regionsOffsetRaw = datum.Read<std::uint16_t>(Offset::DamageRegionsOffset);

		if (regionsOffsetRaw == 0xFFFF) return;
		const std::uint16_t count = regionsSize / k_DamageSectionStride;
		if (count == 0 || count >= 256) return;

		const std::uintptr_t baseAddress = object.Address + regionsOffsetRaw;

		std::vector<DamageSection> raw(count);
		if (!datum.ReadRaw(regionsOffsetRaw, raw.data(), count * sizeof(DamageSection))) return;

		table.BaseAddress = baseAddress;
		table.Sections.resize(count);

		for (std::uint16_t section = 0; section < count; ++section)
		{
			table.Sections[section].DamageLevelMask = raw[section].DamageLevelMask;

			table.Sections[section].Vitality = raw[section].Vitality;
		}
	}

	auto ObjectTableService::ReadRegionBlock(const RawObject& datum, const AliveObject& object, DamageSectionTable& table) -> void
	{
		const std::uint16_t size = datum.Read<std::uint16_t>(Offset::RegionStateSize);
		const std::uint16_t offset = datum.Read<std::uint16_t>(Offset::RegionStateOffset);

		if (offset == 0xFFFF || size == 0 || size > k_RegionBlockMaxSize) return;

		std::vector<std::uint8_t> bytes(size);
		if (!datum.ReadRaw(offset, bytes.data(), size)) return;

		table.Regions.Size = size;
		table.Regions.Offset = offset;
		table.Regions.Bytes = std::move(bytes);
	}

	auto ObjectTableService::UpdateBiped(const RawObject& datum, AliveObject& object) -> void
	{
		BipedObject biped{};

		biped.MovementInput = datum.Read<Vec2>(Offset::Biped::MovementInput);
		biped.VerticalInput = datum.Read<VerticalInput>(Offset::Biped::VerticalInput);
		biped.IsAbilityActive = datum.Read<std::uint8_t>(Offset::Biped::IsAbilityActive);
		biped.ZoomLevel = datum.Read<ZoomLevel>(Offset::Biped::ZoomLevel);

		biped.SurfaceNormal = datum.Read<Vec3>(Offset::Biped::SurfaceNormal);
		biped.GroundObjectHandle = datum.Read<std::uint32_t>(Offset::Biped::GroundObjectHandle);

		biped.DamagerBipedHandle = datum.Read<std::uint32_t>(Offset::Biped::DamagerBipedHandle);
		biped.DamagerPlayerHandle = datum.Read<std::uint32_t>(Offset::Biped::DamagerPlayerHandle);

		object.Specific = biped;
	}

	auto ObjectTableService::UpdateVehicle(const RawObject& datum, AliveObject& object) -> void
	{
		VehicleObject vehicle{};

		vehicle.Base = object.Address;
		vehicle.Kind = Tables::Object::Type::Vehicle::ResolveVehicleType(object.TagName);
		vehicle.SeatLayout = Tables::Object::Type::Vehicle::GetSeatLayout(vehicle.Kind);

		if (TagName::Vehicle::HasBoost(object.TagName))
		{
			vehicle.BoostThrottle = datum.Read<float>(Offset::Vehicle::BoostThrottle);
			vehicle.BoostEnergy = datum.Read<float>(Offset::Vehicle::BoostEnergy);
			vehicle.BoostCooldown = datum.Read<float>(Offset::Vehicle::BoostCooldown);
		}

		object.Specific = vehicle;
	}

	auto ObjectTableService::UpdateWeapon(const RawObject& datum, AliveObject& object) -> void
	{
		WeaponObject weapon{};

		weapon.TotalHeat = datum.Read<float>(Offset::Weapon::TotalHeat);
		weapon.TotalEnergy = 1.0f - datum.Read<float>(Offset::Weapon::TotalEnergy);
		weapon.TotalAmmo = datum.Read<std::uint16_t>(Offset::Weapon::TotalAmmo);
		weapon.CurrentAmmo = datum.Read<std::uint16_t>(Offset::Weapon::CurrentAmmo);

		const std::uint8_t actionByte = datum.Read<std::uint8_t>(Offset::Weapon::ActionState);
		weapon.IsTriggerHeld = (actionByte & k_TriggerHeldBit) != 0;
		weapon.IsZoomed = (actionByte & k_ZoomedBit) != 0;
		weapon.Action = static_cast<WeaponAction>(actionByte & k_ActionMask);

		weapon.IsReloading = datum.Read<std::uint8_t>(Offset::Weapon::IsReloading);
		weapon.ChargeState = datum.Read<ChargeState>(Offset::Weapon::ChargeState);

		weapon.IsTracking = datum.Read<std::uint8_t>(Offset::Weapon::IsTracking);
		weapon.TrackedBipedHandle = datum.Read<std::uint32_t>(Offset::Weapon::TrackedBipedHandle);

		if (object.TagName == TagName::Objective::k_Flag)
		{
			weapon.Team = datum.Read<Team>(Offset::Weapon::Flag::Team);
		}
		else if (object.TagName == TagName::Objective::k_Bomb)
		{
			weapon.Team = datum.Read<Team>(Offset::Weapon::Bomb::Team);
		}

		object.Specific = weapon;
	}

	auto ObjectTableService::UpdateEquipment(const RawObject& datum, AliveObject& object) -> void
	{
		EquipmentObject equipment{};
		equipment.Kind = Tables::Object::Type::Crate::ResolveCrateType(object.TagName);

		equipment.Energy = datum.Read<float>(Offset::Equipment::TotalEnergy);

		if (equipment.Kind == CrateKind::Shield)
		{
			Shield shield{};

			// As far as I know, the only equipment that is a shield,
			// is the drop shield, and it can only be two-way
			if (TagName::Shield::IsTwoWay(object.TagName))
			{
				shield.Kind = ShieldKind::TwoWay;
			}

			equipment.Shield = shield;
		}

		object.Specific = equipment;
	}

	auto ObjectTableService::UpdateProjectiles(const RawObject& datum, AliveObject& object) -> void
	{
		ProjectileObject projectile{};

		projectile.Flags = datum.Read<ProjectileFlags>(Offset::Projectile::RuntimeFlags);
		projectile.OwnerBipedHandle = datum.Read<std::uint32_t>(Offset::Projectile::OwnerBipedHandle);
		projectile.OwnerWeaponHandle = datum.Read<std::uint32_t>(Offset::Projectile::OwnerWeaponHandle);

		object.Specific = projectile;
	}

	auto ObjectTableService::UpdateCrate(const RawObject& datum, AliveObject& object) -> void
	{
		CrateObject crate{};

		crate.Base = object.Address;
		crate.Kind = Tables::Object::Type::Crate::ResolveCrateType(object.TagName);

		if (auto offsets = Tables::Object::Type::Crate::ResolveZoneOffsets(crate.Kind))
		{
			Zone zone{};

			zone.Shape.Radius = datum.Read<float>(offsets->Radius);
			zone.Shape.Length = datum.Read<float>(offsets->Length);
			zone.Shape.Top = datum.Read<float>(offsets->Top);
			zone.Shape.Bottom = datum.Read<float>(offsets->Bottom);
			zone.Shape.Kind = datum.Read<ShapeKind>(offsets->ZoneType);
			zone.Team = datum.Read<Team>(offsets->Team);

			crate.Zone = zone;
		}

		else if (crate.Kind == CrateKind::TeleportSender ||
			crate.Kind == CrateKind::TeleportReceiver ||
			crate.Kind == CrateKind::TeleportTwoWay)
		{
			Teleport teleport{};

			teleport.ZoneShape.Radius = datum.Read<float>(Offset::Crate::Teleport::Radius);
			teleport.ZoneShape.Length = datum.Read<float>(Offset::Crate::Teleport::Length);
			teleport.ZoneShape.Top = datum.Read<float>(Offset::Crate::Teleport::Top);
			teleport.ZoneShape.Bottom = datum.Read<float>(Offset::Crate::Teleport::Bottom);
			teleport.ZoneShape.Kind = datum.Read<ShapeKind>(Offset::Crate::Teleport::ShapeKind);
			teleport.Channel = datum.Read<std::uint8_t>(Offset::Crate::Teleport::Channel);
			teleport.Allowed = datum.Read<Allowed>(Offset::Crate::Teleport::Allowed);

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

	auto ObjectTableService::UpdateScenery(const RawObject& datum, AliveObject& object) -> void
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
				spawn.Team = datum.Read<Team>(Offset::Scenery::SpawnPoint::Team);
			}

			scenery.Spawn = spawn;
		}

		else if (Tables::Object::Type::Scenery::IsBoundary(scenery.Kind))
		{
			Boundary boundary{};

			boundary.Shape.Radius = datum.Read<float>(Offset::Scenery::Boundary::Radius);
			boundary.Shape.Length = datum.Read<float>(Offset::Scenery::Boundary::Length);
			boundary.Shape.Top = datum.Read<float>(Offset::Scenery::Boundary::Top);
			boundary.Shape.Bottom = datum.Read<float>(Offset::Scenery::Boundary::Bottom);
			boundary.Shape.Kind = datum.Read<ShapeKind>(Offset::Scenery::Boundary::ShapeKind);
			boundary.Team = datum.Read<Team>(Offset::Scenery::Boundary::Team);

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