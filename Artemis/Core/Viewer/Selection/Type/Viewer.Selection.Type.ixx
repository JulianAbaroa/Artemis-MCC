export module Viewer.Selection.Type;

import Common.Math.Type;
import std;

export namespace Viewer::Selection::Type
{
    using Vec3 = Common::Math::Type::Vec3;

    // Axis-aligned box of an object in world space.
    struct ObjectBounds
    {
        std::uint32_t Handle{};

        Vec3 Min{};
        Vec3 Max{};
    };

    // Axis-aligned box of a mesh in its own space. Valid is false if the mesh has no triangles.
    struct LocalBounds
    {
        std::array<float, 3> Min{};
        std::array<float, 3> Max{};
        bool Valid{ false };
    };
}