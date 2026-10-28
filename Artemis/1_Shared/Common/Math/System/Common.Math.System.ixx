export module Common.Math.System;

import Common.Math.Type;

namespace
{
    using Common::Math::Type::Vec3;
}

export namespace Common::Math::System
{
    // Returns the sum of two vectors.
    auto Add(const Vec3& a, const Vec3& b) -> Vec3;

    // Returns a minus b.
    auto Subtract(const Vec3& a, const Vec3& b) -> Vec3;

    // Returns the vector multiplied by a scalar.
    auto Scale(const Vec3& v, float scalar) -> Vec3;

    // Returns the cross product of a and b.
    auto Cross(const Vec3& a, const Vec3& b) -> Vec3;

    // Returns the dot product of a and b.
    auto Dot(const Vec3& a, const Vec3& b) -> float;

    // Returns the length of the vector.
    auto Length(const Vec3& v) -> float;

    // Returns the distance between two points.
    auto Distance(const Vec3& a, const Vec3& b) -> float;

    // Returns the vector with length 1.
    // param fallback: Returned if the length is too small to normalize.
    auto Normalize(const Vec3& v, const Vec3& fallback = Vec3{ 0.0f, 0.0f, 1.0f }) -> Vec3;

    // Returns the angle 'from' turned toward 'to' by at most maxStep radians, the short way around, in [0, 2 pi).
    // param to: In [0, 2 pi). Returned as is when it is within maxStep, or when maxStep is 0 or less.
    auto TurnToward(float from, float to, float maxStep) -> float;

    // Returns 'from' moved toward 'to' by at most maxStep.
    // param maxStep: 0 or less returns 'to'.
    auto MoveToward(float from, float to, float maxStep) -> float;

    // Builds an orthonormal frame from a forward direction, using Z as the world up.
    // param normalizeFallback: Used when a direction is too small to normalize.
    // note: If forward is nearly parallel to the world up, the world X axis is used to find the right vector.
    auto BuildFrame(const Vec3& forwardIn, Vec3& outForward, Vec3& outRight,
        Vec3& outUp, const Vec3& normalizeFallback = Vec3{ 0.0f, 0.0f, 0.0f }) -> void;
}