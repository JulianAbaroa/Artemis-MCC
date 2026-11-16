export module Blam.Runtime.Object.System;

import Common.Log.System;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Tag.System;
import Blam.MapModel.Vitality.State;
import Blam.MapModel.World.State;
import Blam.Layout.Snapshot.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import std;

export namespace Blam::Runtime::Object::System
{
    class ObjectTableService
    {
    private:
        using ObjectKind = Blam::MapModel::Definitions::Type::Object::ObjectKind;
        using RawObject = Blam::Layout::Snapshot::Type::RawObject;
        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;
        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using Profile = Blam::Runtime::Object::Type::Profile::Profile;
        using Multiplayer = Blam::Runtime::Object::Type::Multiplayer::Multiplayer;
        using Placement = Blam::Runtime::Object::Type::Placement::Placement;
        using BoneMatrix = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrix;
        using BoneMatrixTable = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
        using BonesHeader = Blam::Runtime::Object::Type::BoneMatrix::BonesHeader;
        using DamageSection = Blam::Runtime::Object::Type::DamageSection::DamageSection;
        using DamageSectionTable = Blam::Runtime::Object::Type::DamageSection::DamageSectionTable;
        using HandleAudit = Blam::Runtime::Object::Type::HandleAudit::HandleAudit;
        using RawCopies = Blam::Runtime::Object::Type::RawCopy::RawCopies;

        using Log = Common::Log::System::Log;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using BoneMatricesStore = Blam::Runtime::Object::State::BoneMatricesStore;
        using DamageSectionsStore = Blam::Runtime::Object::State::DamageSectionsStore;
        using TagResolverService = Blam::MapModel::Tag::System::TagResolverService;

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
        // note: Does not read the game memory. Does nothing while the snapshot has no object table. An object whose datum has the same
        // handle and the same bytes as in the last tick that read it is clean: it is not decoded again and keeps its bone and damage
        // tables. Placement and WorldOrigin are always recomputed (D303).
        auto UpdateObjectTable(const Snapshot& snapshot) -> void;

        // return: What the object links of the last tick resolved to (stale salt, free slot, salt 0). Zero before the first tick.
        // note: AI thread only, the same one that calls UpdateObjectTable.
        auto GetHandleAudit() const -> const HandleAudit&;

        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        BoneMatricesStore& m_BoneMatricesStore;
        DamageSectionsStore& m_DamageSectionsStore;
        TagResolverService& m_TagResolverService;

        HandleAudit m_HandleAudit{};
        // Teardown epoch (Snapshot::TeardownEpoch) the salt 0 WARNING was logged for, or the maximum before the first one: it comes once per
        // map load, not once per tick. AI thread only. A new load runs under a new epoch, so no Cleanup (game thread) has to reset it, and
        // a tick that straggles past the teardown cannot set it for the next load.
        std::uint64_t m_ReportedEpoch{ std::numeric_limits<std::uint64_t>::max() };

        // The datum bytes each slot had in the last tick that read it. AI thread only, so Cleanup (another thread) leaves it: AddObject
        // drops the copy of a new object, which is what makes a new map's objects dirty.
        RawCopies m_RawCopies{};
        // Tick of this service, the one the copies mark the objects they saw in. Starts at 0 and the first tick is 1.
        std::uint64_t m_Tick{};

        // Counts what the object handles the runtime follows resolve to, into m_HandleAudit: the parent, child and next sibling of every
        // object, the weapon slots of the bipeds and vehicles, the owner and shooter of the projectiles, and the bipeds, weapons and
        // objective of the player table.
        // note: A handle whose slot holds another salt is not an error: the object it named was deleted. Only salt 0 is logged.
        auto AuditHandles(const Snapshot& snapshot) -> void;

        // One pass over the objects of the snapshot, by slot: adds the ones that are new (an object of the same slot with another handle
        // leaves), decodes the dirty ones and recomputes Placement and WorldOrigin of all. Marks the rows that changed for Publish.
        // note: Runs inside ObjectTableStore::Edit.
        auto UpdateRows(const Snapshot& snapshot) -> void;

        // Removes the rows of the objects that the pass did not see in the snapshot.
        // note: Runs inside ObjectTableStore::Edit.
        auto RemoveGoneRows() -> void;

        // Compares the datum with the copy of its slot and leaves the copy equal to the datum. Marks the slot as seen in this tick.
        // return: Whether the object is dirty: it has no copy of the same handle or the bytes differ.
        auto TakeRawCopy(const RawObject& datum) -> bool;

        // Adds the row of a snapshot entry.
        // return: The row, or null if the tag of the datum index cannot be resolved.
        // note: Runs inside ObjectTableStore::Edit. A row of another handle in the slot leaves first.
        auto AddObject(std::uint32_t handle, std::uint32_t datumIndex, ObjectKind kind) -> AliveObject*;

        // Takes the row of the handle out of the table and its bone and damage tables.
        // note: Runs inside ObjectTableStore::Edit.
        auto RemoveObject(std::uint32_t handle) -> void;

        auto UpdateObjectData(const RawObject& datum, AliveObject& object, bool isLight) -> void;

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