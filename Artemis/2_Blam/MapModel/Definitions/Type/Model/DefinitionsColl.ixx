export module Blam.MapModel.Definitions.Type:Coll;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Node;
    using Common::Math::Type::Triangle;
}

export namespace Blam::MapModel::Definitions::Type::Coll
{
    // String id of "default", the name of the default permutation of a region.
    inline constexpr std::uint32_t k_DefaultPermutationName{ 1 };

    // Collision triangles of one region and permutation, in the space of its node.
    struct CollMesh
    {
        std::int16_t NodeIndex{ -1 };

        std::int16_t RegionIndex{ -1 };
        std::int16_t PermutationIndex{ -1 };

        std::vector<Triangle> Triangles{};

        Vec3 LocalMin{};
        Vec3 LocalMax{};
    };

    struct Material
    {
        std::uint32_t Name{};
    };

    // Holds the collision model.
    struct Coll
    {
        std::string TagName{};

        std::vector<Node> Nodes{};
        std::vector<CollMesh> Meshes{};

        std::vector<std::uint32_t> RegionNames{};
        std::vector<std::vector<std::uint32_t>> PermutationNames{};
        std::vector<int> DefaultPermutationIndex{};

        Vec3 BoundsMin{};
        Vec3 BoundsMax{};

        // TODO: See if this can be used at all.
        std::vector<Material> Materials{};
    };
}