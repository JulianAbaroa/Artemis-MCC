export module Views.Raycast.Type:Scene;

import :Raycasts;
import :Constant;

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.World.Type;
import Blam.Runtime.Object.Type;
import Perception.Collidable.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::MapModel::Definitions::Type::Coll::CollMesh;
    using Blam::MapModel::World::Type::Raycast::BvhNode;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Perception::Collidable::Type::CollidablePart;
    using Perception::Collidable::Type::CollidablePartsKey;
}

export namespace Views::Raycast::Type
{
    // Collision parts of one object of the tick, for the shared ray scene (D298).
    struct SceneObject
    {
        std::uint32_t Handle{ 0 };

        // At rest in this gather (D300): its parts may go to the fixed tree, which is rebuilt rarely.
        bool IsFixed{ false };

        // The parts the cache of the gather keeps for this object.
        // note: A view, not a copy. Valid until the next gather changes the cache, so nobody keeps a SceneObject past the arming and casting of its tick.
        std::span<const CollidablePart> Parts{};

        // Set by ArmFixedScene: its parts are in the fixed tree and not filtered, so the mobile tree leaves them out.
        bool IsCovered{ false };
    };

    // Every object of the tick that has collision parts, by handle.
    struct SceneObjects : std::vector<SceneObject>
    {
        using vector::vector;
    };

    // A live object of the tick and whether the engine places it as fixed or phased, before its parts are collected.
    // note: Object is the row of the table the gather holds, valid during the gather. Null only in tests with a made-up key function.
    struct SceneRef
    {
        std::uint32_t Handle{ 0 };
        bool IsFixedPhysics{ false };
        const AliveObject* Object{ nullptr };
    };

    // The live objects of the tick.
    struct SceneRefs : std::vector<SceneRef>
    {
        using vector::vector;
    };

    // Parts of an object with the key they were built from (D299, D300).
    // note: IsSeen is the mark of the gather in course: what it does not mark is dropped at its end. StillGathers counts the gathers
    // in a row that found the same key: 0 in the gather where the key changes or the object appears.
    struct SceneParts
    {
        CollidablePartsKey Key{};
        std::vector<CollidablePart> Parts{};
        bool IsSeen{ false };
        std::size_t StillGathers{ 0 };
    };

    // The parts of the objects that the last gather saw, by handle.
    struct ScenePartsCache : std::unordered_map<std::uint32_t, SceneParts>
    {
        using unordered_map::unordered_map;
    };

    // What one gather did, for the offline bench.
    struct SceneGatherCounters
    {
        // Objects that kept their parts because their key was the same.
        std::size_t Reused{ 0 };

        // Objects whose parts were collected again.
        std::size_t Collected{ 0 };
    };

    // Brings the key kept for an object up to date with its row, in place.
    // param isKnown: The key is the one kept for this object. If not, it is empty and the object is new.
    // return: Whether the key differs from what it held.
    using SceneKeyFunction = std::function<bool(const SceneRef&, CollidablePartsKey&, bool)>;

    // Collects the parts of an object, or nothing if it is gone.
    using ScenePartsFunction = std::function<std::optional<std::vector<CollidablePart>>(std::uint32_t)>;

    // Hierarchy over the triangles of one collision mesh, in its local space.
    // note: A leaf counts Order slots from Left. Order holds indices into the triangles of the mesh.
    struct MeshBvh
    {
        std::vector<BvhNode> Nodes{};
        std::vector<std::uint32_t> Order{};
    };

    // Hierarchies of the collision meshes of the map, by identity of the mesh.
    struct MeshBvhs : std::unordered_map<const CollMesh*, MeshBvh>
    {
        using unordered_map::unordered_map;
    };

    // Leaf of an instance tree: one part of one object in the pose of the tick.
    struct ScenePiece
    {
        CachedPart Part{};
        const MeshBvh* Bvh{ nullptr };
        std::uint32_t Handle{ 0 };

        // Position of the part among the parts of its object that rays can hit.
        std::uint32_t PartIndex{ 0 };
    };

    // Hierarchy over the parts of several objects, in the world.
    // note: A leaf counts Order slots from Left. Order holds indices into Pieces.
    struct SceneTree
    {
        std::vector<ScenePiece> Pieces{};
        std::vector<BvhNode> Nodes{};
        std::vector<std::uint32_t> Order{};
    };

    // Identity and pose of a fixed part, to know whether the fixed tree is still valid.
    struct ScenePose
    {
        std::uint32_t Handle{ 0 };
        const CollMesh* Source{ nullptr };
        std::array<float, 12> Transform{};

        auto operator==(const ScenePose&) const -> bool = default;
    };

    // When the fixed tree is built again (D301): the least gathers between builds, the waiting objects that make a build due at once,
    // and the gathers after which any waiting object makes it due.
    struct FixedRebuildRule
    {
        std::size_t Spacing{ Constant::k_FixedRebuildGathers };
        std::size_t Pending{ Constant::k_FixedRebuildPending };
        std::size_t MaxGathers{ Constant::k_FixedRebuildMaxGathers };
    };

    // When the MOBILE tree is built again instead of refitted (D302): the largest growth of the area of its inner nodes since its build.
    struct MobileRebuildRule
    {
        float MaxArea{ Constant::k_MobileRebuildArea };
    };

    // The two level ray scene shared by all agents: a hierarchy per mesh and two hierarchies of instances.
    struct RaycastScene
    {
        // How many times the scene was prepared (PrepareScene).
        std::size_t Prepares{ 0 };

        MeshBvhs Meshes{};
        SceneTree Fixed{};
        SceneTree Mobile{};

        // Poses the fixed tree was built from, by handle.
        std::vector<ScenePose> FixedPoses{};

        // Handles that are in the fixed tree but must not be hit any more (they changed or left rest), sorted. Their current parts are
        // in the mobile tree. Emptied when the fixed tree is built.
        std::vector<std::uint32_t> FixedFilter{};

        // Times the fixed tree was built, the first one included.
        std::size_t FixedBuilds{ 0 };

        // Gathers armed since the last build of the fixed tree.
        std::size_t GathersSinceBuild{ 0 };

        // Gathers in a row that had objects at rest waiting for the fixed tree or handles filtered out of it. Zero when none waits and after a build.
        std::size_t FixedWaitingGathers{ 0 };

        // Times the mobile tree was built (any change in its pieces, or a refit that grew past the rule) and refitted over the same pieces.
        // MobileDegraded counts the builds the rule asked for.
        std::size_t MobileBuilds{ 0 };
        std::size_t MobileRefits{ 0 };
        std::size_t MobileDegraded{ 0 };

        // Sum of the areas of the inner nodes of the mobile tree when it was last built, to measure how much a refit lets it grow.
        float MobileBuiltArea{ 0.0f };

        // Area of the last refit over MobileBuiltArea. The one that asked for a build when it did. 1 after a build for other reasons.
        float MobileAreaRatio{ 1.0f };

        // Objects at rest that are not in the fixed tree yet, and handles filtered out of it, as the last gather found them.
        // note: Kept as they were before a build, which empties the filter. For the bench.
        std::size_t FixedPending{ 0 };
        std::size_t FixedFiltered{ 0 };
    };

    // What a scene had counted when a reach was gathered from it (RV.1): the gathers armed since the last build of the fixed tree, the builds of the
    // fixed tree, and the builds and refits of the mobile one. Any change to the pieces of the trees moves at least one of them.
    struct SceneStamp
    {
        std::size_t Gathers{ 0 };
        std::size_t FixedBuilds{ 0 };
        std::size_t MobileBuilds{ 0 };
        std::size_t MobileRefits{ 0 };

        auto operator==(const SceneStamp&) const -> bool = default;
    };

    // The pieces of the scene that rays from one origin and up to one range can touch (RK.1), for an agent whose rays share their origin.
    // note: The pieces are the ones of the trees of the scene it was gathered from, without the ones the exclusion and the filter of the fixed tree
    // leave out. Valid until the scene is armed again: Stamp and Excluded tell what it was gathered from, so a cast that does not match falls back
    // to the trees (RV.1).
    struct SceneReachPiece
    {
        const ScenePiece* Piece{ nullptr };

        // Box of the part in the world, with a margin, to cull the pieces a ray can not reach before looking at them.
        std::array<float, 3> Min{};
        std::array<float, 3> Max{};
    };

    struct SceneReach
    {
        Vec3 Origin{};
        float Radius{ 0.0f };
        std::vector<SceneReachPiece> Pieces{};

        // The handles of the exclusion it was gathered with, sorted, and what the scene had counted then.
        std::vector<std::uint32_t> Excluded{};
        SceneStamp Stamp{};
    };

    // What one cast of the scene visited, for the offline bench.
    struct SceneCounters
    {
        // Nodes of the instance trees and of the mesh hierarchies together, and the triangles tested.
        std::size_t Nodes{ 0 };
        std::size_t Triangles{ 0 };

        // Profile (RK.1): nodes of the instance trees only, pieces the leaves reached, pieces that passed the exclusion, the filter and the
        // sphere test, and pieces whose mesh was entered after the local box test.
        std::size_t TreeNodes{ 0 };
        std::size_t Pieces{ 0 };
        std::size_t NearPieces{ 0 };
        std::size_t EnteredPieces{ 0 };
    };
}