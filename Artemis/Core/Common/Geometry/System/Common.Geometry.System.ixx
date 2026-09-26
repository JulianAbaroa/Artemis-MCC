export module Common.Geometry.System;

import Common.Math.Type;

export namespace Common::Geometry::System
{
    using Vec3 = Common::Math::Type::Vec3;
    using Triangle = Common::Math::Type::Triangle;

    constexpr float k_Epsilon{ 1e-6f };

    // Möller–Trumbore ray/triangle intersection.
    auto RayIntersectsTriangle(const Vec3& origin, const Vec3& direction,
        const Triangle& triangle, float maxDistance, float& outDistance) -> bool;
}