module;

#include <cmath>

module Common.Math.System;

namespace Common::Math::System
{
    auto Add(const Vec3& a, const Vec3& b) -> Vec3
    {
        return { a.X + b.X, a.Y + b.Y, a.Z + b.Z };
    }

    auto Subtract(const Vec3& a, const Vec3& b) -> Vec3
    {
        return { a.X - b.X, a.Y - b.Y, a.Z - b.Z };
    }

    auto Scale(const Vec3& v, float scalar) -> Vec3
    {
        return { v.X * scalar, v.Y * scalar, v.Z * scalar };
    }

    auto Dot(const Vec3& a, const Vec3& b) -> float
    {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    }

    auto Cross(const Vec3& a, const Vec3& b) -> Vec3
    {
        return {
            a.Y * b.Z - a.Z * b.Y,
            a.Z * b.X - a.X * b.Z,
            a.X * b.Y - a.Y * b.X
        };
    }

    auto Length(const Vec3& v) -> float
    {
        return std::sqrt(Dot(v, v));
    }

    auto Distance(const Vec3& a, const Vec3& b) -> float
    {
        return Length(Subtract(a, b));
    }

    auto Normalize(const Vec3& v, const Vec3& fallback) -> Vec3
    {
        const float length = Length(v);
        if (length < 1e-6f) return fallback;

        return Scale(v, 1.0f / length);
    }

    auto BuildFrame(const Vec3& forwardIn, Vec3& outForward, Vec3& outRight,
        Vec3& outUp, const Vec3& normalizeFallback) -> void
    {
        constexpr Vec3 kWorldUp{ 0.0f, 0.0f, 1.0f };
        constexpr float kParallelEpsilon = 1e-4f;

        outForward = Normalize(forwardIn, normalizeFallback);

        Vec3 right = Cross(outForward, kWorldUp);

        if (Dot(right, right) < kParallelEpsilon)
        {
            constexpr Vec3 kWorldX{ 1.0f, 0.0f, 0.0f };
            right = Cross(outForward, kWorldX);
        }

        outRight = Normalize(right, normalizeFallback);
        outUp = Cross(outRight, outForward);
    }
}