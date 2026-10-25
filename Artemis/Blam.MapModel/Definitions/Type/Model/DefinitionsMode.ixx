export module Resolved.Definitions.Type:Mode;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Vec4;
    using Common::Math::Type::Node;
}

export namespace Resolved::Definitions::Type::Mode
{
    struct Marker
    {
        std::int8_t NodeIndex{ -1 };
        std::uint8_t Flags{};

        Vec3 Translation{};
        Vec4 Rotation{};
        Vec3 Direction{};
        float Scale{ 1.0f };
    };

    struct MarkerGroup
    {
        std::uint32_t NameId{};
        std::vector<Marker> Markers{};
    };

    // Permutations a region of the render model can draw.
    struct Region
    {
        std::uint32_t NameId{};
        std::vector<std::uint32_t> PermutationNames{};

        // Meshes each permutation draws, so a permutation with none shows nothing.
        std::vector<int> PermutationMeshCounts{};
    };

    struct Bounds
    {
        Vec3 Min{};
        Vec3 Max{};
    };

    // Holds the render model.
    struct Mode
    {
        std::string TagName{};
        std::vector<MarkerGroup> MarkerGroups{};
        std::vector<Node> Nodes{};

        // The engine keeps the permutation of an object by index in these regions.
        std::vector<Region> Regions{};

        Bounds ModelBounds{};
    };
}