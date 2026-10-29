export module Perception.Collidable.System;

import Common.Math.Type;
import Common.Log.System;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.World.Type;
import Blam.MapModel.World.State;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Perception.Classifier.Type;
import Perception.Classifier.State;
import Perception.Collidable.Type;
import std;

export namespace Perception::Collidable::System
{
    class CollidableService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using CollMesh = Blam::MapModel::Definitions::Type::Coll::CollMesh;
        using Coll = Blam::MapModel::Definitions::Type::Coll::Coll;
        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using BoneMatrix = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrix;
        using BoneMatrixTable = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
        using DamageSectionTable = Blam::Runtime::Object::Type::DamageSection::DamageSectionTable;
        using Classified = Perception::Classifier::Type::Classified;
        using Collidable = Perception::Collidable::Type::Collidable;
        using CollidablePart = Perception::Collidable::Type::CollidablePart;
        using Collidables = Perception::Collidable::Type::Collidables;
        using Context = Perception::Collidable::Type::Context;

        using Log = Common::Log::System::Log;
        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using BoneMatricesStore = Blam::Runtime::Object::State::BoneMatricesStore;
        using DamageSectionsStore = Blam::Runtime::Object::State::DamageSectionsStore;
        using WorldStore = Blam::MapModel::World::State::WorldStore;
        using ClassifierStore = Perception::Classifier::State::ClassifierStore;

    public:
        CollidableService(Log& log, DefinitionsStore& definitionsStore,
            ObjectTableStore& objectTableStore, BoneMatricesStore& boneMatricesStore,
            DamageSectionsStore& damageSectionsStore, ClassifierStore& classifierStore,
            WorldStore& worldStore) : m_Log(log),
            m_DefinitionsStore(definitionsStore), m_ObjectTableStore(objectTableStore),
            m_BoneMatricesStore(boneMatricesStore), m_DamageSectionsStore(damageSectionsStore),
            m_ClassifierStore(classifierStore),
            m_WorldStore(worldStore) {}
        ~CollidableService() = default;

        // Builds the collision meshes of every live object.
        // return: The whole scene, or nothing while the classifier or the object table has no data yet.
        // note: For tools (D224). Same thread as the pipeline, it reads stores that the AI thread writes.
        auto BuildScene() -> std::optional<Collidables>;

        // Builds one object with its parts only.
        // note: Each part is a collision mesh with the transform that places it in the world.
        auto CollectPartsFor(std::uint32_t handle) -> std::optional<Collidable>;

        auto QueryNearby(const Vec3& origin, float radius) const -> std::vector<std::uint32_t>;

        // Logs the cleanup with the rest of the layer. The scene it builds is owned by its caller (D224), so there is no state to clear.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        DefinitionsStore& m_DefinitionsStore;
        ObjectTableStore& m_ObjectTableStore;
        BoneMatricesStore& m_BoneMatricesStore;
        DamageSectionsStore& m_DamageSectionsStore;
        ClassifierStore& m_ClassifierStore;
        WorldStore& m_WorldStore;

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