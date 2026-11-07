export module Blam.MapModel.Definitions.Type:Sbsp;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Triangle;
}

export namespace Blam::MapModel::Definitions::Type::Sbsp
{
    // Holds the bounds, the render triangles and the collision triangles of a structure bsp.
    struct Sbsp
    {
        std::string TagName{};

        Vec3 WorldBoundsMin{};
        Vec3 WorldBoundsMax{};
        Vec3 MoppBoundsMin{};
        Vec3 MoppBoundsMax{};

        std::vector<Triangle> RenderGeometry{};

        // Collision surfaces of the structure and of the placed instanced geometry, all in world space, as triangles.
        // SurfaceFlags are the flags of the collision surface (two sided, invisible, climbable, breakable...) and Material is the
        // global material index of the collision material of the surface (-1 if it has none).
        std::vector<Triangle> CollisionGeometry{};
    };
}