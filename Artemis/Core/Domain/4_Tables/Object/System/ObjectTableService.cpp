module Tables.Object.System;

import Common.Math.Type;
import Common.Semantic.Type;
import Common.ZoneShape.Type;
import Common.Tag.Type;
import Common.Team.Type;
import Resolved.Definitions.Type;
import Datum.Object.Type;

namespace
{
	namespace TagName = Common::Tag::Type;
    namespace ObjeOffset = Datum::Object::Type::Obje::Offset;
	namespace MvarOffset = Datum::Object::Type::Mvar::Offset;

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
	using GrenadeType = Tables::Object::Type::Biped::GrenadeType;
	using Damage = Tables::Object::Type::Biped::Damage;
	using DamageReportingType = Resolved::Definitions::Type::Object::DamageReportingType;
	using Shield = Tables::Object::Type::Crate::Shield::Shield;
	using Placement = Tables::Object::Type::Placement::Placement;
	using Symmetry = Tables::Object::Type::Placement::Symmetry;
	using Physics = Common::Semantic::Type::Physics;

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
	constexpr std::size_t k_VehicleBoostThrottle{ UnitOffset::k_Unknown53c };
	constexpr std::size_t k_VehicleBoostCooldown{ UnitOffset::k_Unknown544 };

	// Damage slots of a unit and the bytes between two of them.
	constexpr std::size_t k_DamageSlots{ 4 };
	constexpr std::size_t k_DamageSlotStride{ UnitOffset::k_Damage1Tick - UnitOffset::k_Damage0Tick };

	// Weapon slots of a unit, as ActiveWeaponSlot indexes them, and the bytes between two handles.
	constexpr std::uint8_t k_WeaponSlots{ 4 };
	constexpr std::size_t k_WeaponSlotStride{ sizeof(std::uint32_t) };

	// Value of an empty handle or tick.
	constexpr std::uint32_t k_None{ 0xFFFFFFFF };

	// Nodes a model can have. Descriptors above it are not plausible.
	constexpr std::uint32_t k_MaxNodes{ 1024 };

	// Bit of the placement flags (Mvar +0x00) set when the slot holds a placement.
	constexpr std::uint16_t k_PlacementInUseBit{ 0x0001 };

	// Bits of the Forge flags of a placement (Mvar +0x46). Symmetry takes two bits from k_SymmetryShift.
	// note: Phased sets both physics bits (0xC0), Fixed only 0x40.
	constexpr std::uint8_t k_NotPlacedAtStartBit{ 0x02 };
	constexpr std::uint8_t k_SymmetryShift{ 2 };
	constexpr std::uint8_t k_SymmetryMask{ 0x03 };
	constexpr std::uint8_t k_GameSpecificBit{ 0x20 };
	constexpr std::uint8_t k_FixedBit{ 0x40 };
	constexpr std::uint8_t k_PhasedBit{ 0x80 };

	// Color of a placement left on the Forge default.
	constexpr std::uint8_t k_DefaultColor{ 0xFF };

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
				object.Placement = this->ReadPlacement(*snapshot, it->second);
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

		object.BoundingCenter = datum.Read<Vec3>(ObjeOffset::k_BoundingCenter);
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

	auto ObjectTableService::ReadPlacement(const Snapshot& snapshot, const RawObject& datum) -> std::optional<Placement>
	{
		const std::uint16_t index = datum.Read<std::uint16_t>(ObjeOffset::k_VariantPlacementIndex);
		if (index == 0xFFFF) return std::nullopt;

		const std::optional<std::uint16_t> flags = snapshot.ReadPlacement<std::uint16_t>(index, MvarOffset::k_PlacementFlags);
		if (!flags || (*flags & k_PlacementInUseBit) == 0) return std::nullopt;

		const std::uint8_t forgeFlags = snapshot.ReadPlacement<std::uint8_t>(index, MvarOffset::k_ForgeFlags).value_or(0);

		Placement placement{};
		placement.Index = index;

		if ((forgeFlags & k_PhasedBit) != 0) placement.Physics = Physics::Phased;
		else if ((forgeFlags & k_FixedBit) != 0) placement.Physics = Physics::Fixed;
		else placement.Physics = Physics::Normal;

		placement.Symmetry = static_cast<Symmetry>((forgeFlags >> k_SymmetryShift) & k_SymmetryMask);
		placement.IsPlacedAtStart = (forgeFlags & k_NotPlacedAtStartBit) == 0;
		placement.IsGameSpecific = (forgeFlags & k_GameSpecificBit) != 0;

		placement.SpawnSequence = snapshot.ReadPlacement<std::int8_t>(index, MvarOffset::k_SpawnSequence).value_or(0);
		placement.SpawnTime = snapshot.ReadPlacement<std::uint8_t>(index, MvarOffset::k_SpawnTime).value_or(0);
		placement.Color = snapshot.ReadPlacement<std::uint8_t>(index, MvarOffset::k_Color).value_or(k_DefaultColor);

		return placement;
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

		biped.TicksAlive = datum.Read<std::uint32_t>(UnitOffset::k_TicksAlive);
		biped.Aim = datum.Read<Vec3>(UnitOffset::k_CurrentAim);

		biped.MovementInput = datum.Read<Vec2>(UnitOffset::k_Throttle);
		// note: The two bytes the repo read before are the low bytes of the control flags.
		const std::uint64_t controlFlags = datum.Read<std::uint64_t>(UnitOffset::k_ControlFlags);
		biped.VerticalInput = static_cast<VerticalInput>(controlFlags & 0xFF);
		biped.IsAbilityActive = static_cast<std::uint8_t>((controlFlags >> 8) & 0xFF);
		biped.ZoomLevel = datum.Read<ZoomLevel>(UnitOffset::k_ZoomLevel);

		biped.SurfaceNormal = datum.Read<Vec3>(BipdOffset::k_SurfaceNormal);
		biped.GroundObjectHandle = datum.Read<std::uint32_t>(BipdOffset::k_GroundObjectHandle);

		biped.PrimaryWeaponHandle = datum.Read<std::uint32_t>(UnitOffset::k_PrimaryWeaponHandle);
		biped.SecondaryWeaponHandle = datum.Read<std::uint32_t>(UnitOffset::k_SecondaryWeaponHandle);

		// note: The slot reads 0xFF in a vehicle, where the engine hides the weapons.
		const std::uint8_t slot{ datum.Read<std::uint8_t>(UnitOffset::k_ActiveWeaponSlot) };
		biped.EquippedWeaponHandle = slot < k_WeaponSlots
			? datum.Read<std::uint32_t>(UnitOffset::k_PrimaryWeaponHandle + (slot * k_WeaponSlotStride))
			: k_None;

		biped.ArmorAbilityHandle = datum.Read<std::uint32_t>(UnitOffset::k_ArmorAbilityHandle);

		biped.SelectedGrenade = datum.Read<GrenadeType>(UnitOffset::k_SelectedGrenadeType);
		biped.FragGrenades = datum.Read<std::uint8_t>(UnitOffset::k_FragGrenadeCount);
		biped.PlasmaGrenades = datum.Read<std::uint8_t>(UnitOffset::k_PlasmaGrenadeCount);

		for (std::size_t i{ 0 }; i < k_DamageSlots; ++i)
		{
			const std::size_t slotOffset{ i * k_DamageSlotStride };

			const std::uint32_t tick{ datum.Read<std::uint32_t>(UnitOffset::k_Damage0Tick + slotOffset) };
			if (tick == k_None) continue;

			biped.Damages.push_back(Damage{
				.Tick = tick,
				.Health = datum.Read<float>(UnitOffset::k_Damage0Health + slotOffset),
				.Shield = datum.Read<float>(UnitOffset::k_Damage0Shield + slotOffset),
				.BipedHandle = datum.Read<std::uint32_t>(UnitOffset::k_Damage0BipedHandle + slotOffset),
				.PlayerHandle = datum.Read<std::uint32_t>(UnitOffset::k_Damage0PlayerHandle + slotOffset),
				.ReportingType = static_cast<DamageReportingType>(datum.Read<std::uint16_t>(UnitOffset::k_Damage0ReportingType + slotOffset)),
			});
		}

		biped.LastDamageTick = datum.Read<std::uint32_t>(UnitOffset::k_LastDamageTick);

		// note: The game clock of a biped (obje 0xD8) is the current tick on every update, so it needs no global.
		const std::uint32_t gameTick{ datum.Read<std::uint32_t>(ObjeOffset::k_LastUpdateTick) };
		if (biped.LastDamageTick != k_None && gameTick >= biped.LastDamageTick)
		{
			biped.TicksSinceDamage = gameTick - biped.LastDamageTick;
		}

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

		object.Specific = weapon;
	}

	auto ObjectTableService::UpdateEquipment(const RawObject& datum, AliveObject& object) -> void
	{
		EquipmentObject equipment{};
		equipment.Kind = Tables::Object::Type::Crate::ResolveCrateType(object.TagName);

		equipment.Energy = datum.Read<float>(EqipOffset::k_Energy);
		equipment.IsActive = datum.Read<std::uint32_t>(EqipOffset::k_ActivationTick) != k_None;

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

		if (crate.Kind == CrateKind::TeleportSender ||
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

		object.Specific = scenery;
	}

	auto ObjectTableService::Cleanup() -> void
	{
		m_ObjectStore.Cleanup();

		m_LogsService.Message("[ObjectTableService] INFO: Cleanup completed.");
	}
}