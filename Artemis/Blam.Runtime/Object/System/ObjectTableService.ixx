export module Tables.Object.System;

import Common.Log.System;
import Resolved.Definitions.Type;
import Resolved.Tag.System;
import Resolved.Vitality.State;
import Resolved.World.State;
import Layout.Snapshot.Type;
import Tables.Object.Type;
import Tables.Object.State;
import std;

export namespace Tables::Object::System
{
    class ObjectTableService
    {
    private:
        using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
        using RawObject = Layout::Snapshot::Type::RawObject;
        using Snapshot = Layout::Snapshot::Type::Snapshot;
        using AliveObject = Tables::Object::Type::Alive::AliveObject;
        using Profile = Tables::Object::Type::Profile::Profile;
        using Multiplayer = Tables::Object::Type::Multiplayer::Multiplayer;
        using Placement = Tables::Object::Type::Placement::Placement;
        using BoneMatrix = Tables::Object::Type::BoneMatrix::BoneMatrix;
        using BoneMatrixTable = Tables::Object::Type::BoneMatrix::BoneMatrixTable;
        using BonesHeader = Tables::Object::Type::BoneMatrix::BonesHeader;
        using DamageSection = Tables::Object::Type::DamageSection::DamageSection;
        using DamageSectionTable = Tables::Object::Type::DamageSection::DamageSectionTable;

        using Log = Common::Log::System::Log;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using BoneMatricesStore = Tables::Object::State::BoneMatricesStore;
        using DamageSectionsStore = Tables::Object::State::DamageSectionsStore;
        using TagResolverService = Resolved::Tag::System::TagResolverService;

    public:
        ObjectTableService(Log& log,
            ObjectTableStore& objectStore,
            BoneMatricesStore& boneMatricesStore,
            DamageSectionsStore& damageSectionsStore,
            TagResolverService& tagResolverService) :
            m_Log(log),
            m_ObjectStore(objectStore),
            m_BoneMatricesStore(boneMatricesStore),
            m_DamageSectionsStore(damageSectionsStore),
            m_TagResolverService(tagResolverService) {}
        ~ObjectTableService() = default;

        // Builds the alive objects from the snapshot of the tick.
        // note: Does not read the game memory. Does nothing while the snapshot has no object table.
        auto UpdateObjectTable(const Snapshot& snapshot) -> void;

        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        BoneMatricesStore& m_BoneMatricesStore;
        DamageSectionsStore& m_DamageSectionsStore;
        TagResolverService& m_TagResolverService;

        // Adds the objects of the snapshot that are not stored yet.
        auto DiscoverObjects(const Snapshot& snapshot) -> void;

        // Adds the object of a snapshot entry.
        // note: Does nothing if the tag of the datum index cannot be resolved.
        auto AddObject(std::uint32_t handle, std::uint32_t datumIndex, ObjectKind kind) -> void;

        auto UpdateObjectData(const RawObject& datum, AliveObject& object) -> void;

        // Reads the position and size of the bone block from the descriptor in the datum (0x178).
        // return: The header, or nothing if the object has no bone block or the descriptor is not plausible.
        auto ResolveBonesHeader(const RawObject& datum) -> std::optional<BonesHeader>;

        auto ReadBoneMatrixTable(const RawObject& datum, AliveObject& object) -> void;
        auto ReadDamageSectionTable(const RawObject& datum, AliveObject& object) -> void;
        auto ReadDamageSections(const RawObject& datum, DamageSectionTable& table) -> void;

        // Reads the block with the damage state of each region.
        // note: Skipped when the header has no block or its size is not plausible.
        auto ReadRegionBlock(const RawObject& datum, DamageSectionTable& table) -> void;

        // Reads the zone shape and the team from the multiplayer block (0x190).
        // return: The data, or nothing if the object does not have the block.
        auto ReadMultiplayer(const RawObject& datum) -> std::optional<Multiplayer>;

        // Reads the Forge settings from the placement of the map variant that the datum points to (obje 0x1C).
        // return: The settings, or nothing if the object was not placed by the map variant or its placement is empty.
        auto ReadPlacement(const Snapshot& snapshot, const RawObject& datum) -> std::optional<Placement>;

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