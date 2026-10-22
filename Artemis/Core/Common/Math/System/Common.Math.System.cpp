module Common.Math.System;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;

    using Common::Math::Type::k_WorldUp;
    using Common::Math::Type::k_TwoPi;

    constexpr Vec3 k_WorldX{ 1.0f, 0.0f, 0.0f };
    constexpr float k_ParallelEpsilon{ 1e-4f };
}

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

    auto TurnToward(float from, float to, float maxStep) -> float
    {
        if (maxStep <= 0.0f) return to;

        // Signed difference in [-pi, pi]: the short way from 'from' to 'to'.
        const float delta{ std::remainder(to - from, k_TwoPi) };
        if (std::fabs(delta) <= maxStep) return to;

        float angle{ std::fmod(from + std::copysign(maxStep, delta), k_TwoPi) };
        if (angle < 0.0f) angle += k_TwoPi;

        // A tiny negative angle rounds to 2 pi when wrapped.
        return angle < k_TwoPi ? angle : 0.0f;
    }

    auto MoveToward(float from, float to, float maxStep) -> float
    {
        if (maxStep <= 0.0f) return to;
        return from + std::clamp(to - from, -maxStep, maxStep);
    }

    auto BuildFrame(const Vec3& forwardIn, Vec3& outForward, Vec3& outRight,
        Vec3& outUp, const Vec3& normalizeFallback) -> void
    {
        outForward = Normalize(forwardIn, normalizeFallback);

        Vec3 right = Cross(outForward, k_WorldUp);

        if (Dot(right, right) < k_ParallelEpsilon)
        {
            right = Cross(outForward, k_WorldX);
        }

        outRight = Normalize(right, normalizeFallback);
        outUp = Cross(outRight, outForward);
    }
}