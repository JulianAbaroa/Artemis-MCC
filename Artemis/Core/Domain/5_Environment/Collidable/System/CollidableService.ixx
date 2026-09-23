export module Environment.Collidable.System;

import Service.Logs.System;
import Common.Math.Type;
import Resolved.World.Type;
import Resolved.World.State;
import Tables.Object.Type;
import Tables.Object.State;
import Relations.Classifier.State;
import Relations.ObjectGraph.State;
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
        using CollMesh = Resolved::World::Type::Coll::Mesh;
        using ResolvedColl = Resolved::World::Type::Coll::Coll;
        using ResolvedRegionStates = Resolved::World::Type::RegionStates::RegionStates;
        using StateMap = std::vector<std::array<int, 5>>;
        using AliveObject = Tables::Object::Type::Alive::Object;
        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using BoneMatrix = Tables::Object::Type::BoneMatrix::BoneMatrix;
        using BoneMatrixTable = Tables::Object::Type::BoneMatrix::BoneMatrixTable;
        using DamageSectionTable = Tables::Object::Type::DamageSection::DamageSectionTable;
        using Classified = Relations::Classifier::Type::Classified;
        using Classifieds = std::vector<Classified>;
        using Collidable = Environment::Collidable::Type::Collidable;
        using Context = Environment::Collidable::Type::Context;

        using LogsService = Service::Logs::System::LogsService;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using BoneMatricesStore = Tables::Object::State::BoneMatricesStore;
        using DamageSectionsStore = Tables::Object::State::DamageSectionsStore;
        using WorldStore = Resolved::World::State::WorldStore;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;
        using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;
        using CollidableStore = Environment::Collidable::State::CollidableStore;

    public:
        CollidableService(LogsService& logsService, ObjectTableStore& objectTableStore,
            BoneMatricesStore& boneMatricesStore, 
            DamageSectionsStore& damageSectionsStore, ClassifierStore& classifierStore,
            ObjectGraphStore& objectGraphStore, WorldStore& worldStore,
            CollidableStore& collidableStore) : m_LogsService(logsService),
            m_ObjectTableStore(objectTableStore), m_BoneMatricesStore(boneMatricesStore),
            m_DamageSectionsStore(damageSectionsStore), 
            m_ClassifierStore(classifierStore), m_ObjectGraphStore(objectGraphStore),
            m_WorldStore(worldStore), m_CollidableStore(collidableStore) {}
        ~CollidableService() = default;

        auto Update() -> void;

        auto IsBoneMatrixValid(const BoneMatrix& m) -> bool;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        ObjectTableStore& m_ObjectTableStore;
        BoneMatricesStore& m_BoneMatricesStore;
        DamageSectionsStore& m_DamageSectionsStore;
        ClassifierStore& m_ClassifierStore;
        ObjectGraphStore& m_ObjectGraphStore;
        WorldStore& m_WorldStore;
        CollidableStore& m_CollidableStore;

        auto CollectCollidables(const Classifieds& classifieds,
            const ObjectTable& objects) -> void;

        auto CollectMesh(const Collidable& instance, const Context& ctx,
            const BoneMatrixTable* boneMatrixTable,
            const DamageSectionTable* damageSectionTable) -> CollMesh;

        auto CollectSkeletal(const Collidable& instance, const Context& ctx,
            const BoneMatrixTable& boneMatrixTable,
            const DamageSectionTable* damageSectionTable) -> CollMesh;

        auto CollectRigid(const Collidable& instance, const Context& ctx,
            const DamageSectionTable* damageSectionTable) -> CollMesh;

        auto IsActivePermutation(const Collidable& instance, const Context& ctx,
            const CollMesh& mesh, const DamageSectionTable* damageSectionTable) -> bool;

        // --- Helpers ---

        auto Cross(const Vec3& a, const Vec3& b) -> Vec3;

        auto TransformPoint(const Vec3& pos,
            const Vec3& right, const Vec3& forward,
            const Vec3& up, float lx, float ly, float lz) -> Vec3;

        auto TransformByBone(const BoneMatrix& m, float lx, float ly, float lz) -> Vec3;

        auto HighestLevelFromMask(std::uint16_t mask) -> int;

        auto IsAncestorDead(std::uint32_t handle, const ObjectTable& objects) -> bool;
    };
}