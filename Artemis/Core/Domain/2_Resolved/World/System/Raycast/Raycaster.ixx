export module Resolved.World.System:Raycast;

import Common.Math.Type;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Resolved.World.Type;
import std;

export namespace Resolved::World::System
{
    // Casts rays against the static geometry of the map, using a bounding volume hierarchy.
    class Raycaster
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using Sbsp = Resolved::Definitions::Type::Sbsp::Sbsp;
        using Hit = Resolved::World::Type::Raycast::Hit;
        using BvhNode = Resolved::World::Type::Raycast::BvhNode;
        using Triangle = Common::Math::Type::Triangle;

    public:
        Raycaster() = default;
        ~Raycaster() = default;

        // Indexes the render triangles of every sbsp in a hierarchy.
        // note: Runs once per map, after the definitions are built. The triangles are copied, so the store is not referenced afterwards.
        auto Build(const DefinitionsStore& definitionsStore) -> void;

        // Finds the closest triangle a ray hits.
        // param direction: Unit vector.
        // param maxDistance: Rays that would travel further miss.
        auto Cast(const Vec3& origin, const Vec3& direction, float maxDistance) const -> Hit;

        // Removes the hierarchy.
        auto Cleanup() -> void;

    private:
        std::vector<BvhNode> m_Nodes{};
        std::vector<Triangle> m_Triangles{};

        auto BuildBvh(const std::vector<const Sbsp*>& sbsps) -> void;
    };
}