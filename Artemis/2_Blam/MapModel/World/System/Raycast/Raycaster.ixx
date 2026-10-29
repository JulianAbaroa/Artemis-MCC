export module Blam.MapModel.World.System:Raycast;

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.World.Type;
import std;

export namespace Blam::MapModel::World::System
{
    // Casts rays against the static geometry of the map, using a bounding volume hierarchy.
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

        // Builds the hierarchy over the render triangles of the sbsps.
        auto BuildBvh(const std::vector<const Sbsp*>& sbsps) -> void;
    };
}