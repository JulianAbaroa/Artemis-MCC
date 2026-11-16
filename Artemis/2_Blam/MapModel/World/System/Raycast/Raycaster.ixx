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

    // Fits the boxes of a hierarchy built by BuildBoundsBvh to new boxes of the same primitives, keeping its topology (D302).
    // param bounds: The new box of each primitive, in the order BuildBoundsBvh was given them. Its size is the one of order.
    // param order: What BuildBoundsBvh returned for the hierarchy.
    // return: BoundsBvhArea of the refitted hierarchy.
    // note: Each box ends equal to the one a build over the same boxes would give the same node, so nothing a node holds is lost.
    // Only the quality of the hierarchy can get worse as the primitives move away from where it was built. Nothing changes if the sizes
    // do not match the hierarchy.
    auto RefitBoundsBvh(const std::vector<Blam::MapModel::World::Type::Raycast::BvhBounds>& bounds, const std::vector<std::uint32_t>& order,
        std::vector<Blam::MapModel::World::Type::Raycast::BvhNode>& nodes) -> float;

    // return: The sum of the surface areas of the inner nodes of a hierarchy, the measure of how well it is built. Zero without inner nodes.
    auto BoundsBvhArea(const std::vector<Blam::MapModel::World::Type::Raycast::BvhNode>& nodes) -> float;

    // Whether a ray enters the box of a node before tMax.
    // param inverse: InverseDirection of each component of the direction.
    // param enter: The distance at which the ray enters the box.
    auto RayEntersNode(const Blam::MapModel::World::Type::Raycast::BvhNode& node, const std::array<float, 3>& origin,
        const std::array<float, 3>& inverse, float tMax, float& enter) -> bool;

    // return: The inverse of a component of a direction, finite when the component is zero.
    auto InverseDirection(float component) -> float;
}