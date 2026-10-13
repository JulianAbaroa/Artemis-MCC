module Tables.Object.System;

import Common.Math.Type;
import Common.ZoneShape.Type;
import Common.Tag.Type;
import Common.Team.Type;
import Resolved.Definitions.Type;
import Datum.Object.Type;

namespace
{
	namespace TagName = Common::Tag::Type;
    namespace ObjeOffset = Datum::Object::Type::Obje::Offset;

	using Vec2 = Common::Math::Type::Vec2;
	using Vec3 = Common::Math::Type::Vec3;
	using ShapeKind = Common::ZoneShape::Type::Kind;
	using Team = Common::Team::Type::Team;
	using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
	using RawObject = Datum::Object::Type::Raw::RawObject;
	using Snapshot = Datum::Object::Type::Raw::Snapshot;
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

	// note: The origin of a parented object is local to its parent, so it is carried
	// through the parent chain. Halo frames are forward, left (up x forward) and up.
	template <typename Raws>
	auto ResolveWorldOrigin(const Raws& raws, const RawObject& datum, int depth = 0) -> Vec3
	{
		const Vec3 local = datum.Read<Vec3>(ObjeOffset::k_Origin);
		const std::uint32_t parentHandle = datum.Read<std::uint32_t>(ObjeOffset::k_ParentHandle);

		if (parentHandle == 0xFFFFFFFFu || depth >= 8) return local;

		auto parentIt = raws.find(parentHandle);
		if (parentIt == raws.end()) return local;

		const RawObject& parent = parentIt->second;
		const Vec3 parentOrigin = ResolveWorldOrigin(raws, parent, depth + 1);
		const Vec3 forward = parent.Read<Vec3>(ObjeOffset::k_Forward);
		const Vec3 up = parent.Read<Vec3>(ObjeOffset::k_Up);
		const Vec3 left{
			up.Y * forward.Z - up.Z * forward.Y,
			up.Z * forward.X - up.X * forward.Z,
			up.X * forward.Y - up.Y * forward.X };

		return Vec3{
			parentOrigin.X + forward.X * local.X + left.X * local.Y + up.X * local.Z,
			parentOrigin.Y + forward.Y * local.X + left.Y * local.Y + up.Y * local.Z,
			parentOrigin.Z + forward.Z * local.X + left.Z * local.Y + up.Z * local.Z };
	}

	using Tables::Object::Type::Constant::k_DamageSectionStride;
	using Tables::Object::Type::Constant::k_RegionBlockMaxSize;

	using Datum::Object::Type::Constant::k_MinDatumSize;

	namespace ObjeOffset = Datum::Object::Type::Obje::Offset;
	namespace UnitOffset = Datum::Object::Type::Unit::Offset;
	namespace BipdOffset = Datum::Object::Type::Bipd::Offset;
	namespace WeapOffset = Datum::Object::Type::Weap::Offset;
	namespace ProjOffset = Datum::Object::Type::Proj::Offset;
	namespace EqipOffset = Datum::Object::Type::Eqip::Offset;
	namespace SeatBlock = Datum::Object::Type::Block::Seat;
	namespace MultiplayerBlock = Datum::Object::Type::Block::Multiplayer;

	// Fields that are not confirmed in the datum layouts yet. They keep the name they had before the migration.
	constexpr std::size_t k_ProjectileFlags{ ProjOffset::k_Unknown1a8 };
	constexpr std::size_t k_ProjectileOwnerBipedHandle{ ProjOffset::k_Unknown1c4 };
	constexpr std::size_t k_ProjectileOwnerWeaponHandle{ ProjOffset::k_Unknown1d8 };
	constexpr std::size_t k_EquipmentTotalEnergy{ EqipOffset::k_Unknown1ec };
	constexpr std::size_t k_VehicleBoostThrottle{ UnitOffset::k_Unknown53c };
	constexpr std::size_t k_VehicleBoostCooldown{ UnitOffset::k_Unknown544 };

	// Nodes a model can have. Descriptors above it are not plausible.
	constexpr std::uint32_t k_MaxNodes{ 1024 };

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

			std::uint32_t datumIndex{ datum.Read<std::uint32_t>(ObjeOffset::k_TagHandle) };

			this->AddObject(handle, datumIndex, datum.Kind);
		}
	}

	auto ObjectTableService::UpdateObjectTable() -> void
	{
		auto snapshot = m_DatumStore.Acquire();
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
				object.WorldOrigin = ResolveWorldOrigin(snapshot->RawObjects, it->second);
			});

		for (std::uint32_t handle : deadHandles)
		{
			m_ObjectStore.RemoveObject(handle);
		}

		m_ObjectStore.Publish();
	}

	auto ObjectTableService::UpdateObjectData(const RawObject& datum, AliveObject& object) -> void
	{
		object.NextSiblingHandle = datum.Read<std::uint32_t>(ObjeOffset::k_NextSiblingHandle);
		object.ChildHandle = datum.Read<std::uint32_t>(ObjeOffset::k_ChildHandle);
		object.ParentHandle = datum.Read<std::uint32_t>(ObjeOffset::k_ParentHandle);

		object.Position = datum.Read<Vec3>(ObjeOffset::k_BoundingCenter);
		object.Origin = datum.Read<Vec3>(ObjeOffset::k_Origin);
		object.Forward = datum.Read<Vec3>(ObjeOffset::k_Forward);
		object.Up = datum.Read<Vec3>(ObjeOffset::k_Up);
		object.LinearVelocity = datum.Read<Vec3>(ObjeOffset::k_LinearVelocity);
		object.AngularVelocity = datum.Read<Vec3>(ObjeOffset::k_AngularVelocity);

		object.BoundingRadius = datum.Read<float>(ObjeOffset::k_BoundingRadius);
		object.DamageReceived = datum.Read<float>(ObjeOffset::k_DamageReceived);

		object.HlmtVariant = datum.Read<std::uint16_t>(ObjeOffset::k_ModelVariant);

		object.Multiplayer = this->ReadMultiplayer(datum);

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

	auto ObjectTableService::ReadMultiplayer(const RawObject& datum) -> std::optional<Multiplayer>
	{
		if (!datum.FindBlock(MultiplayerBlock::k_Descriptor)) return std::nullopt;

		Multiplayer multiplayer{};

		multiplayer.Shape.Radius = datum.ReadBlock<float>(MultiplayerBlock::k_Descriptor, MultiplayerBlock::k_Radius);
		multiplayer.Shape.Length = datum.ReadBlock<float>(MultiplayerBlock::k_Descriptor, MultiplayerBlock::k_Length);
		multiplayer.Shape.Top = datum.ReadBlock<float>(MultiplayerBlock::k_Descriptor, MultiplayerBlock::k_Top);
		multiplayer.Shape.Bottom = datum.ReadBlock<float>(MultiplayerBlock::k_Descriptor, MultiplayerBlock::k_Bottom);
		multiplayer.Shape.Kind = datum.ReadBlock<ShapeKind>(MultiplayerBlock::k_Descriptor, MultiplayerBlock::k_ShapeKind);
		multiplayer.Team = datum.ReadBlock<Team>(MultiplayerBlock::k_Descriptor, MultiplayerBlock::k_Team);

		return multiplayer;
	}

	auto ObjectTableService::ResolveBonesHeader(const RawObject& datum) -> std::optional<BonesHeader>
	{
		const std::uint16_t size = datum.Read<std::uint16_t>(ObjeOffset::k_NodeMatricesSize);
		const std::uint16_t offset = datum.Read<std::uint16_t>(ObjeOffset::k_NodeMatricesOffset);

		if (offset == 0xFFFF || offset < k_MinDatumSize) return std::nullopt;
		if (size == 0 || size % sizeof(BoneMatrix) != 0) return std::nullopt;

		const std::uint32_t nodeCount = static_cast<std::uint32_t>(size / sizeof(BoneMatrix));
		if (nodeCount >= k_MaxNodes) return std::nullopt;

		BonesHeader header{};
		header.Offset = offset;
		header.NodeCount = nodeCount;

		return header;
	}

	auto ObjectTableService::ReadBoneMatrixTable(const RawObject& datum, AliveObject& object) -> void
	{
		const std::optional<BonesHeader> header = this->ResolveBonesHeader(datum);
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
		const std::uint16_t regionsSize = datum.Read<std::uint16_t>(ObjeOffset::k_DamageSectionsSize);
		const std::uint16_t regionsOffsetRaw = datum.Read<std::uint16_t>(ObjeOffset::k_DamageSectionsOffset);

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
		const std::uint16_t size = datum.Read<std::uint16_t>(ObjeOffset::k_RegionStateSize);
		const std::uint16_t offset = datum.Read<std::uint16_t>(ObjeOffset::k_RegionStateOffset);

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

		biped.MovementInput = datum.Read<Vec2>(UnitOffset::k_Throttle);
		// note: The two bytes the repo read before are the low bytes of the control flags.
		const std::uint64_t controlFlags = datum.Read<std::uint64_t>(UnitOffset::k_ControlFlags);
		biped.VerticalInput = static_cast<VerticalInput>(controlFlags & 0xFF);
		biped.IsAbilityActive = static_cast<std::uint8_t>((controlFlags >> 8) & 0xFF);
		biped.ZoomLevel = datum.Read<ZoomLevel>(UnitOffset::k_ZoomLevel);

		biped.SurfaceNormal = datum.Read<Vec3>(BipdOffset::k_SurfaceNormal);
		biped.GroundObjectHandle = datum.Read<std::uint32_t>(BipdOffset::k_GroundObjectHandle);

		biped.DamagerBipedHandle = datum.Read<std::uint32_t>(UnitOffset::k_Damage0BipedHandle);
		biped.DamagerPlayerHandle = datum.Read<std::uint32_t>(UnitOffset::k_Damage0PlayerHandle);

		object.Specific = biped;
	}

	auto ObjectTableService::UpdateVehicle(const RawObject& datum, AliveObject& object) -> void
	{
		VehicleObject vehicle{};

		vehicle.Base = object.Address;
		vehicle.Kind = Tables::Object::Type::Vehicle::ResolveVehicleType(object.TagName);
		vehicle.SeatLayout = Tables::Object::Type::Vehicle::GetSeatLayout(vehicle.Kind);

		if (const auto seats = datum.FindBlock(SeatBlock::k_Descriptor))
		{
			const std::size_t seatCount = seats->Size / SeatBlock::k_ElementSize;

			vehicle.SeatAvailability.reserve(seatCount);
			for (std::size_t seat{ 0 }; seat < seatCount; ++seat)
			{
				vehicle.SeatAvailability.push_back(datum.ReadBlock<float>(
					SeatBlock::k_Descriptor, (seat * SeatBlock::k_ElementSize) + SeatBlock::k_Target));
			}
		}

		if (TagName::Vehicle::HasBoost(object.TagName))
		{
			vehicle.BoostThrottle = datum.Read<float>(k_VehicleBoostThrottle);
			vehicle.BoostEnergy = datum.Read<float>(UnitOffset::k_BoostValue);
			vehicle.BoostCooldown = datum.Read<float>(k_VehicleBoostCooldown);
		}

		object.Specific = vehicle;
	}

	auto ObjectTableService::UpdateWeapon(const RawObject& datum, AliveObject& object) -> void
	{
		WeaponObject weapon{};

		weapon.TotalHeat = datum.Read<float>(WeapOffset::k_TotalHeat);
		weapon.TotalEnergy = 1.0f - datum.Read<float>(WeapOffset::k_TotalEnergy);
		weapon.TotalAmmo = datum.Read<std::uint16_t>(WeapOffset::k_Magazine0TotalAmmo);
		weapon.CurrentAmmo = datum.Read<std::uint16_t>(WeapOffset::k_Magazine0CurrentAmmo);

		const std::uint8_t actionByte = static_cast<std::uint8_t>(datum.Read<std::uint16_t>(WeapOffset::k_InputMask));
		weapon.IsTriggerHeld = (actionByte & k_TriggerHeldBit) != 0;
		weapon.IsZoomed = (actionByte & k_ZoomedBit) != 0;
		weapon.Action = static_cast<WeaponAction>(actionByte & k_ActionMask);

		weapon.IsReloading = static_cast<std::uint8_t>(datum.Read<std::uint16_t>(WeapOffset::k_Magazine0ReloadState));
		weapon.ChargeState = datum.Read<ChargeState>(WeapOffset::k_Trigger0ChargeState);

		weapon.IsTracking = datum.Read<std::uint8_t>(WeapOffset::k_IsTracking);
		weapon.TrackedBipedHandle = datum.Read<std::uint32_t>(WeapOffset::k_TrackedBipedHandle);

		if ((object.TagName == TagName::Objective::k_Flag ||
			object.TagName == TagName::Objective::k_Bomb) && object.Multiplayer)
		{
			weapon.Team = object.Multiplayer->Team;
		}

		object.Specific = weapon;
	}

	auto ObjectTableService::UpdateEquipment(const RawObject& datum, AliveObject& object) -> void
	{
		EquipmentObject equipment{};
		equipment.Kind = Tables::Object::Type::Crate::ResolveCrateType(object.TagName);

		equipment.Energy = datum.Read<float>(k_EquipmentTotalEnergy);

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

		projectile.Flags = datum.Read<ProjectileFlags>(k_ProjectileFlags);
		projectile.OwnerBipedHandle = datum.Read<std::uint32_t>(k_ProjectileOwnerBipedHandle);
		projectile.OwnerWeaponHandle = datum.Read<std::uint32_t>(k_ProjectileOwnerWeaponHandle);

		object.Specific = projectile;
	}

	auto ObjectTableService::UpdateCrate(const RawObject& datum, AliveObject& object) -> void
	{
		CrateObject crate{};

		crate.Base = object.Address;
		crate.Kind = Tables::Object::Type::Crate::ResolveCrateType(object.TagName);

		if (Tables::Object::Type::Crate::HasZone(crate.Kind))
		{
			Zone zone{};

			if (object.Multiplayer)
			{
				zone.Shape = object.Multiplayer->Shape;
				zone.Team = object.Multiplayer->Team;
			}

			crate.Zone = zone;
		}

		else if (crate.Kind == CrateKind::TeleportSender ||
			crate.Kind == CrateKind::TeleportReceiver ||
			crate.Kind == CrateKind::TeleportTwoWay)
		{
			Teleport teleport{};

			if (object.Multiplayer) teleport.ZoneShape = object.Multiplayer->Shape;
			teleport.Channel = datum.ReadBlock<std::uint8_t>(MultiplayerBlock::k_Descriptor, MultiplayerBlock::k_Channel);
			teleport.Allowed = datum.ReadBlock<Allowed>(MultiplayerBlock::k_Descriptor, MultiplayerBlock::k_Allowed);

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
				if (object.Multiplayer) spawn.Team = object.Multiplayer->Team;
			}

			scenery.Spawn = spawn;
		}

		else if (Tables::Object::Type::Scenery::IsBoundary(scenery.Kind))
		{
			Boundary boundary{};

			if (object.Multiplayer)
			{
				boundary.Shape = object.Multiplayer->Shape;
				boundary.Team = object.Multiplayer->Team;
			}

			scenery.Boundary = boundary;
		}

		object.Specific = scenery;
	}

	auto ObjectTableService::Cleanup() -> void
	{
		m_ObjectStore.Cleanup();

		m_LogsService.Message("[ObjectTableService] INFO: Cleanup completed.");
	}
}