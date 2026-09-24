export module Tables.Object.System;

import Service.Logs.System;
import Platform.Memory.System;
import Map.Reader.System;
import Resolved.Stats.State;
import Resolved.Vitality.State;
import Resolved.World.State;
import Tables.Object.Type;
import Tables.Object.State;
import std;

export namespace Tables::Object::System
{
	class ObjectTableService
	{
	private:
		using AliveObject = Tables::Object::Type::Alive::Object;
		using Profile = Tables::Object::Type::Profile::Profile;
		using BoneMatrix = Tables::Object::Type::BoneMatrix::BoneMatrix;
		using BoneMatrixTable = Tables::Object::Type::BoneMatrix::BoneMatrixTable;
		using DamageSection = Tables::Object::Type::DamageSection::DamageSection;
		using DamageSectionTable = Tables::Object::Type::DamageSection::DamageSectionTable;

		using LogsService = Service::Logs::System::LogsService;
		using MemoryReaderService = Platform::Memory::System::MemoryReaderService;
		using ObjectStore = Tables::Object::State::ObjectTableStore;
		using BoneOffsetsStore = Tables::Object::State::BoneOffsetsStore;
		using BoneMatricesStore = Tables::Object::State::BoneMatricesStore;
		using DamageSectionsStore = Tables::Object::State::DamageSectionsStore;
		using TagResolverService = Map::Reader::System::TagResolverService;

		static constexpr std::uint8_t m_kDamageSectionStride = 0x18;

	public:
		ObjectTableService(LogsService& logsService,
			MemoryReaderService& memoryReaderService,
			ObjectStore& objectStore, BoneOffsetsStore& boneOffsetsStore,
			BoneMatricesStore& boneMatricesStore,
			DamageSectionsStore& damageSectionsStore,
			TagResolverService& tagResolverService) :
			m_LogsService(logsService), m_MemoryReaderService(memoryReaderService),
			m_ObjectStore(objectStore),
			m_BoneOffsetsStore(boneOffsetsStore), m_BoneMatricesStore(boneMatricesStore),
			m_DamageSectionsStore(damageSectionsStore),
			m_TagResolverService(tagResolverService) {
		}
		~ObjectTableService() = default;

		auto OnObjectCreated(std::uint32_t handle, std::uint32_t datumIndex) -> void;
		auto OnObjectDestroyed(std::uint32_t handle) -> void;

		auto UpdateObjectTable() -> void;

		auto Cleanup() -> void;

	private:
		LogsService& m_LogsService;
		MemoryReaderService& m_MemoryReaderService;
		ObjectStore& m_ObjectStore;
		BoneOffsetsStore& m_BoneOffsetsStore;
		BoneMatricesStore& m_BoneMatricesStore;
		DamageSectionsStore& m_DamageSectionsStore;
		TagResolverService& m_TagResolverService;

		void UpdateObjectData(AliveObject& object);

		auto ReadBoneMatrixTable(MemoryReaderService& reader, AliveObject& object) -> void;
		auto ReadDamageSectionTable(MemoryReaderService& reader, AliveObject& object) -> void;

		auto UpdateBiped(MemoryReaderService& reader, AliveObject& object) -> void;
		auto UpdateVehicle(MemoryReaderService& reader, AliveObject& object) -> void;
		auto UpdateWeapon(MemoryReaderService& reader, AliveObject& object) -> void;
		auto UpdateEquipment(MemoryReaderService& reader, AliveObject& object) -> void;
		auto UpdateProjectiles(MemoryReaderService& reader, AliveObject& object) -> void;
		auto UpdateCrate(MemoryReaderService& reader, AliveObject& object) -> void;
		auto UpdateScenery(MemoryReaderService& reader, AliveObject& object) -> void;

		auto SetProfile(AliveObject& object, Profile& profile) -> void;
	};
}