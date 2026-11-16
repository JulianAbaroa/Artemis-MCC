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

    // Slots of the stack that traverses a hierarchy.
    inline constexpr int k_StackSize{ 128 };

    // A ray prepared to be tested against many boxes: its origin and the inverse of its direction, in four lanes (the fourth is not used).
    struct RayLanes
    {
        std::array<float, 4> Origin{};
        std::array<float, 4> Inverse{};
    };

    // Node of the bounding volume hierarchy.
    // note: An inner node has its children at Left and Left + 1. A leaf has Count triangles starting at Left.
    struct BvhNode
    {
        std::array<float, 3> Min{};
        std::array<float, 3> Max{};
        std::int32_t Left{};
        std::int32_t Count{};
    };

    // Box of one primitive of a hierarchy, before the hierarchy is built.
    struct BvhBounds
    {
        std::array<float, 3> Min{};
        std::array<float, 3> Max{};
    };

    // Closest collision surface a ray hits.
    // note: SurfaceFlags are the flags of the collision surface (two sided, invisible, climbable, breakable...) and Material is
    // its global material index, -1 if it has none. Both are the defaults when nothing was hit.
    struct Hit
    {
        bool IsHit{};
        float Distance{};
        Vec3 Point{};
        std::uint8_t SurfaceFlags{ 0 };
        std::int16_t Material{ -1 };
    };
}