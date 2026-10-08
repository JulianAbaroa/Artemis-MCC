export module Environment.Collidable.System;

import Service.Logs.System;
import Common.Math.Type;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Resolved.World.Type;
import Resolved.World.State;
import Tables.Object.Type;
import Tables.Object.State;
import Relations.Classifier.State;
import Relations.Classifier.Type;
import Environment.Collidable.Type;
import Environment.Collidable.State;
import std;

export namespace Environment::Collidable::System
{
    class CollidableService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using CollMesh = Resolved::Definitions::Type::Coll::Mesh;
        using ResolvedColl = Resolved::Definitions::Type::Coll::Coll;
        using AliveObject = Tables::Object::Type::Alive::Object;
        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using BoneMatrix = Tables::Object::Type::BoneMatrix::BoneMatrix;
        using BoneMatrixTable = Tables::Object::Type::BoneMatrix::BoneMatrixTable;
        using DamageSectionTable = Tables::Object::Type::DamageSection::DamageSectionTable;
        using Classified = Relations::Classifier::Type::Classified;
        using Classifieds = std::vector<Classified>;
        using Collidable = Environment::Collidable::Type::Collidable;
        using CollidablePart = Environment::Collidable::Type::CollidablePart;
        using Context = Environment::Collidable::Type::Context;

        using LogsService = Service::Logs::System::LogsService;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using BoneMatricesStore = Tables::Object::State::BoneMatricesStore;
        using DamageSectionsStore = Tables::Object::State::DamageSectionsStore;
        using WorldStore = Resolved::World::State::WorldStore;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;
        using CollidableStore = Environment::Collidable::State::CollidableStore;

    public:
        CollidableService(LogsService& logsService, DefinitionsStore& definitionsStore,
            ObjectTableStore& objectTableStore, BoneMatricesStore& boneMatricesStore,
            DamageSectionsStore& damageSectionsStore, ClassifierStore& classifierStore,
            WorldStore& worldStore, CollidableStore& collidableStore) : m_LogsService(logsService),
            m_DefinitionsStore(definitionsStore), m_ObjectTableStore(objectTableStore),
            m_BoneMatricesStore(boneMatricesStore), m_DamageSectionsStore(damageSectionsStore),
            m_ClassifierStore(classifierStore),
            m_WorldStore(worldStore), m_CollidableStore(collidableStore) {}
        ~CollidableService() = default;

        auto Update(bool isDebugViewActive) -> void;

        // Builds one object with its parts only.
        // note: Each part is a collision mesh with the transform that places it in the world.
        auto CollectPartsFor(std::uint32_t handle) -> std::optional<Collidable>;

        auto QueryNearby(const Vec3& origin, float radius) const -> std::vector<std::uint32_t>;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        DefinitionsStore& m_DefinitionsStore;
        ObjectTableStore& m_ObjectTableStore;
        BoneMatricesStore& m_BoneMatricesStore;
        DamageSectionsStore& m_DamageSectionsStore;
        ClassifierStore& m_ClassifierStore;
        WorldStore& m_WorldStore;
        CollidableStore& m_CollidableStore;

        auto CollectCollidables(const Classifieds& classifieds,
            const ObjectTable& objects) -> void;

        auto BuildInstance(const AliveObject& object, bool buildWorldMesh) -> Collidable;

        auto CollectMesh(const Collidable& instance, const Context& ctx,
            const BoneMatrixTable* boneMatrixTable,
            const DamageSectionTable* damageSectionTable) -> CollMesh;

        auto CollectSkeletal(const Context& ctx, const BoneMatrixTable& boneMatrixTable,
            const DamageSectionTable* damageSectionTable) -> CollMesh;

        auto CollectRigid(const Collidable& instance, const Context& ctx,
            const DamageSectionTable* damageSectionTable) -> CollMesh;

        auto CollectParts(const Collidable& instance, const Context& ctx,
            const BoneMatrixTable* boneMatrixTable,
            const DamageSectionTable* damageSectionTable) -> std::vector<CollidablePart>;

        // Tells if the collision mesh belongs to the permutation the engine shows for its region.
        // note: Objects without a region block show the default permutation of each region.
        auto IsActivePermutation(const Context& ctx, const CollMesh& mesh,
            const DamageSectionTable* damageSectionTable) -> bool;

        // Fills the engine state of each region of the render model next to what the viewer shows, for diagnostics.
        // note: The block keeps one entry per region of the render model. Its last bytes are still unknown.
        auto BuildRegionDiagnostics(Collidable& instance, const Context& ctx,
            const DamageSectionTable* damageSectionTable) -> void;

        // --- Helpers ---

        auto Cross(const Vec3& a, const Vec3& b) -> Vec3;

        auto TransformPoint(const Vec3& pos,
            const Vec3& right, const Vec3& forward,
            const Vec3& up, float lx, float ly, float lz) -> Vec3;

        auto TransformByBone(const BoneMatrix& m, float lx, float ly, float lz) -> Vec3;
    };
}