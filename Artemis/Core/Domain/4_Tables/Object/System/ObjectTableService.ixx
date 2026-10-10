export module Tables.Object.System;

import Service.Logs.System;
import Map.Reader.System;
import Resolved.Definitions.Type;
import Resolved.Vitality.State;
import Resolved.World.State;
import Template.Object.State;
import Template.Object.Type;
import Tables.Object.Type;
import Tables.Object.State;
import std;

export namespace Tables::Object::System
{
	class ObjectTableService
	{
	private:
        using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
		using RawObject = Template::Object::Type::Raw::RawObject;
		using Snapshot = Template::Object::Type::Raw::Snapshot;
		using AliveObject = Tables::Object::Type::Alive::Object;
		using Profile = Tables::Object::Type::Profile::Profile;
		using BoneMatrix = Tables::Object::Type::BoneMatrix::BoneMatrix;
		using BoneMatrixTable = Tables::Object::Type::BoneMatrix::BoneMatrixTable;
		using DamageSection = Tables::Object::Type::DamageSection::DamageSection;
		using DamageSectionTable = Tables::Object::Type::DamageSection::DamageSectionTable;

		using LogsService = Service::Logs::System::LogsService;
		using TemplateStore = Template::Object::State::TemplateStore;
		using ObjectStore = Tables::Object::State::ObjectTableStore;
		using BoneOffsetsStore = Tables::Object::State::BoneOffsetsStore;
		using BoneMatricesStore = Tables::Object::State::BoneMatricesStore;
		using DamageSectionsStore = Tables::Object::State::DamageSectionsStore;
		using TagResolverService = Map::Reader::System::TagResolverService;

	public:
		ObjectTableService(LogsService& logsService,
			TemplateStore& templateStore,
			ObjectStore& objectStore, BoneOffsetsStore& boneOffsetsStore,
			BoneMatricesStore& boneMatricesStore,
			DamageSectionsStore& damageSectionsStore,
			TagResolverService& tagResolverService) :
			m_LogsService(logsService), m_TemplateStore(templateStore),
			m_ObjectStore(objectStore),
			m_BoneOffsetsStore(boneOffsetsStore), m_BoneMatricesStore(boneMatricesStore),
			m_DamageSectionsStore(damageSectionsStore),
			m_TagResolverService(tagResolverService) {}
		~ObjectTableService() = default;

		// Builds the alive objects from the last snapshot of the template layer.
		// note: Does not read the game memory. Does nothing until a snapshot is published.
		auto UpdateObjectTable() -> void;

		auto Cleanup() -> void;

	private:
		LogsService& m_LogsService;
		TemplateStore& m_TemplateStore;
		ObjectStore& m_ObjectStore;
		BoneOffsetsStore& m_BoneOffsetsStore;
		BoneMatricesStore& m_BoneMatricesStore;
		DamageSectionsStore& m_DamageSectionsStore;
		TagResolverService& m_TagResolverService;

		// Adds the objects of the snapshot that are not stored yet.
		auto DiscoverObjects(const Snapshot& snapshot) -> void;

		// Adds the object of a snapshot entry.
		// note: Does nothing if the tag of the datum index cannot be resolved.
		auto AddObject(std::uint32_t handle, std::uint32_t datumIndex, ObjectKind kind) -> void;

		auto UpdateObjectData(const RawObject& datum, AliveObject& object) -> void;

		auto ReadBoneMatrixTable(const RawObject& datum, AliveObject& object) -> void;
		auto ReadDamageSectionTable(const RawObject& datum, AliveObject& object) -> void;
		auto ReadDamageSections(const RawObject& datum, const AliveObject& object, DamageSectionTable& table) -> void;

		// Reads the block with the damage state of each region.
		// note: Skipped when the header has no block or its size is not plausible.
		auto ReadRegionBlock(const RawObject& datum, const AliveObject& object, DamageSectionTable& table) -> void;

		auto UpdateBiped(const RawObject& datum, AliveObject& object) -> void;
		auto UpdateVehicle(const RawObject& datum, AliveObject& object) -> void;
		auto UpdateWeapon(const RawObject& datum, AliveObject& object) -> void;
		auto UpdateEquipment(const RawObject& datum, AliveObject& object) -> void;
		auto UpdateProjectiles(const RawObject& datum, AliveObject& object) -> void;
		auto UpdateCrate(const RawObject& datum, AliveObject& object) -> void;
		auto UpdateScenery(const RawObject& datum, AliveObject& object) -> void;

		auto SetProfile(AliveObject& object, Profile& profile) -> void;
	};
}