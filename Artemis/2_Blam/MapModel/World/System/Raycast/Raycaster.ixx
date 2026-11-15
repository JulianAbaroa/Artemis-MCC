export module Blam.MapModel.World.System:Raycast;

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.World.Type;
import std;

export namespace Blam::MapModel::World::System
{
    // Casts rays against the static collision of the map (structure and instanced geometry), using a bounding volume hierarchy.
    class Raycaster
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using Sbsp = Blam::MapModel::Definitions::Type::Sbsp::Sbsp;
        using Hit = Blam::MapModel::World::Type::Raycast::Hit;
        using BvhNode = Blam::MapModel::World::Type::Raycast::BvhNode;
        using Triangle = Common::Math::Type::Triangle;

    public:
        Raycaster() = default;
        ~Raycaster() = default;

        // Indexes the collision triangles of every sbsp in a hierarchy.
        // note: Runs once per map, after the definitions are built. The triangles are copied, so the store is not referenced afterwards.
        auto Build(const DefinitionsStore& definitionsStore) -> void;

        // Finds the closest collision surface a ray hits.
        // note: The ray stops at the first surface it crosses, whatever its type (invisible and two sided included), and either face
        // counts. The hit carries the flags and the global material of the surface (D272).
        // param direction: Unit vector.
        // param maxDistance: Rays that would travel further miss.
        auto Cast(const Vec3& origin, const Vec3& direction, float maxDistance) const -> Hit;

        // Removes the hierarchy.
        auto Cleanup() -> void;

    private:
        std::vector<BvhNode> m_Nodes{};
        std::vector<Triangle> m_Triangles{};

        // Builds the hierarchy over the collision triangles of the sbsps.
        auto BuildBvh(const std::vector<const Sbsp*>& sbsps) -> void;
    };

    // Builds a hierarchy over boxes, with the same surface area heuristic the Raycaster uses for its triangles (D298).
    // param order: Receives, for each slot the leaves count from, the index in bounds of the box that sits there. Empty if there are no boxes.
    auto BuildBoundsBvh(const std::vector<Blam::MapModel::World::Type::Raycast::BvhBounds>& bounds,
        std::vector<Blam::MapModel::World::Type::Raycast::BvhNode>& nodes, std::vector<std::uint32_t>& order) -> void;

    // Whether a ray enters the box of a node before tMax.
    // param inverse: InverseDirection of each component of the direction.
    // param enter: The distance at which the ray enters the box.
    auto RayEntersNode(const Blam::MapModel::World::Type::Raycast::BvhNode& node, const std::array<float, 3>& origin,
        const std::array<float, 3>& inverse, float tMax, float& enter) -> bool;

    // return: The inverse of a component of a direction, finite when the component is zero.
    auto InverseDirection(float component) -> float;
}