export module Views.Raycast.Type:Scene;

import :Raycasts;

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.World.Type;
import Perception.Collidable.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Coll::CollMesh;
    using Blam::MapModel::World::Type::Raycast::BvhNode;
    using Perception::Collidable::Type::CollidablePart;
    using Perception::Collidable::Type::CollidablePartsKey;
}

export namespace Views::Raycast::Type
{
    // Collision parts of one object of the tick, for the shared ray scene (D298).
    struct SceneObject
    {
        std::uint32_t Handle{ 0 };

        // Fixed and phased objects do not move: their parts go to the tree that is rebuilt only when they change.
        bool IsFixed{ false };

        std::vector<CollidablePart> Parts{};
    };

    // Every object of the tick that has collision parts, by handle.
    struct SceneObjects : std::vector<SceneObject>
    {
        using vector::vector;
    };

    // A live object of the tick and whether it is fixed, before its parts are collected.
    struct SceneRef
    {
        std::uint32_t Handle{ 0 };
        bool IsFixed{ false };
    };

    // The live objects of the tick.
    struct SceneRefs : std::vector<SceneRef>
    {
        using vector::vector;
    };

    // Parts of a fixed object with the key they were built from (D299).
    // note: IsSeen is the mark of the gather in course: what it does not mark is dropped at its end.
    struct FixedParts
    {
        CollidablePartsKey Key{};
        std::vector<CollidablePart> Parts{};
        bool IsSeen{ false };
    };

    // The parts of the fixed objects that the last gather kept, by handle.
    struct FixedPartsCache : std::unordered_map<std::uint32_t, FixedParts>
    {
        using unordered_map::unordered_map;
    };

    // Fills the key of the parts of an object, given the key kept for it or null. Gives false if the object is gone.
    using SceneKeyFunction = std::function<bool(std::uint32_t, CollidablePartsKey&, const CollidablePartsKey*)>;

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

    // The two level ray scene shared by all agents: a hierarchy per mesh and two hierarchies of instances.
    struct RaycastScene
    {
        MeshBvhs Meshes{};
        SceneTree Fixed{};
        SceneTree Mobile{};

        // Poses the fixed tree was built from.
        std::vector<ScenePose> FixedPoses{};

        // Times the fixed tree was built, the first one included.
        std::size_t FixedBuilds{ 0 };
    };

    // What one cast of the scene visited, for the offline bench.
    struct SceneCounters
    {
        std::size_t Nodes{ 0 };
        std::size_t Triangles{ 0 };
    };
}