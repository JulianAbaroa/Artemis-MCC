export module Common.Geometry.System;

import Common.Math.Type;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Triangle = Common::Math::Type::Triangle;
}

export namespace Common::Geometry::System
{
    // Tests a ray against a triangle (Moller-Trumbore).
    // param maxDistance: Hits farther than this are ignored.
    // param outDistance: Distance along the ray to the hit. Only written on a hit.
    // return: False if the ray misses, is parallel to the triangle or the hit is out of range.
    auto RayIntersectsTriangle(const Vec3& origin, const Vec3& direction,
        const Triangle& triangle, float maxDistance, float& outDistance) -> bool;
}