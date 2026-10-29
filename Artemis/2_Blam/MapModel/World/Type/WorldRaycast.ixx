export module Blam.MapModel.World.Type:Raycast;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
}

export namespace Blam::MapModel::World::Type::Raycast
{
    // Padding of every box of a ray tree, so rays that graze a triangle edge are not lost to rounding.
    inline constexpr float k_BoxPadding{ 1.0e-3f };

    // Node of the bounding volume hierarchy.
    // note: An inner node has its children at Left and Left + 1. A leaf has Count triangles starting at Left.
    struct BvhNode
    {
        std::array<float, 3> Min{};
        std::array<float, 3> Max{};
        std::int32_t Left{};
        std::int32_t Count{};
    };

    // Closest triangle a ray hits.
    struct Hit
    {
        bool IsHit{};
        float Distance{};
        Vec3 Point{};
    };
}