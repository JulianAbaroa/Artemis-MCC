module Common.Geometry.System;
import Common.Math.System;

namespace
{
    using namespace Common::Math::System;
}

namespace Common::Geometry::System
{
    auto RayIntersectsTriangle(const Vec3& origin, const Vec3& direction,
        const Triangle& triangle, float maxDistance, float& outDistance) -> bool
    {
        const Vec3 edge1 = Subtract(triangle.B, triangle.A);
        const Vec3 edge2 = Subtract(triangle.C, triangle.A);

        const Vec3 pvec = Cross(direction, edge2);
        const float det = Dot(edge1, pvec);

        if (det > -k_Epsilon && det < k_Epsilon) return false;

        const float invDet = 1.0f / det;
        const Vec3 tvec = Subtract(origin, triangle.A);

        const float u = Dot(tvec, pvec) * invDet;
        if (u < 0.0f || u > 1.0f) return false;

        const Vec3 qvec = Cross(tvec, edge1);
        const float v = Dot(direction, qvec) * invDet;
        if (v < 0.0f || u + v > 1.0f) return false;

        const float t = Dot(edge2, qvec) * invDet;
        if (t <= k_Epsilon || t > maxDistance) return false;

        outDistance = t;
        return true;
    }
}