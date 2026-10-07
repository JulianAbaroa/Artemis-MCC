export module Resolved.Definitions.Type:Coll;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Node;
    using Common::Math::Type::Triangle;
}

export namespace Resolved::Definitions::Type::Coll
{
    // Collision triangles of one region and permutation, in the space of its node.
    struct Mesh
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
        std::vector<Mesh> Meshes{};

        std::vector<std::uint32_t> RegionNames{};
        std::vector<std::vector<std::uint32_t>> PermutationNames{};
        std::vector<int> DefaultPermutationIndex{};

        Vec3 BoundsMin{};
        Vec3 BoundsMax{};

        // TODO: See if this can be used at all.
        std::vector<Material> Materials{};
    };
}