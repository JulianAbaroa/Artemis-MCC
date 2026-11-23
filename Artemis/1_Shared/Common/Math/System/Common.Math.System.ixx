export module Common.Math.System;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Pose;
    using Common::Math::Type::RenderPose;
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Vec4;
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

    // Returns whether the three components are numbers (neither NaN nor infinite).
    auto IsFinite(const Vec3& v) -> bool;

    // Returns the vector rotated by a unit quaternion (x, y, z, w).
    auto Rotate(const Vec4& quaternion, const Vec3& v) -> Vec3;

    // Returns the distance between two points.
    auto Distance(const Vec3& a, const Vec3& b) -> float;

    // Moves a point with a 3x4 matrix of 12 floats: three rows of rotation and scale, then the translation.
    // note: Row vector convention, as the instance transforms of the map file.
    auto TransformPoint3x4(const Vec3& point, const float* matrix) -> Vec3;

    // Returns the vector with length 1.
    // param fallback: Returned if the length is too small to normalize.
    auto Normalize(const Vec3& v, const Vec3& fallback = Vec3{ 0.0f, 0.0f, 1.0f }) -> Vec3;

    // Builds an orthonormal frame from a forward direction, using Z as the world up.
    // param normalizeFallback: Used when a direction is too small to normalize.
    // note: If forward is nearly parallel to the world up, the world X axis is used to find the right vector.
    auto BuildFrame(const Vec3& forwardIn, Vec3& outForward, Vec3& outRight,
        Vec3& outUp, const Vec3& normalizeFallback = Vec3{ 0.0f, 0.0f, 0.0f }) -> void;

    // Returns the pose a fraction of the way from a to b: the position in a line, the axes blended and made unit and at right angles again.
    // param t: 0 gives a, 1 gives b, past 1 goes on at the same pace (extrapolation). A blend that collapses (opposite axes) keeps the axes of a.
    auto BlendPose(const Pose& a, const Pose& b, float t) -> Pose;

    // Returns the pose to draw an object with between ticks, as the game draws it (VW.5e, D330).
    // param previous: Pose in the tick the frame shows (T - 1).
    // param newer: Pose in the next tick (T), if that tick is known.
    // param older: Pose in the tick before the shown one (T - 2), if known.
    // param fraction: How far the game is from the shown tick to the next, 0 to 1 (a value outside it, or not a number, gives None).
    // param snapDistance: A step between two ticks longer than this (teleport, respawn, new seat) is not blended.
    // return: Between previous and newer when newer is there (Exact), else previous moved on at the pace of older to previous (Extrapolated),
    // else previous (None). Past the snap distance, or with a pose that is not finite, also previous.
    auto ResolveRenderPose(const Pose& previous, const std::optional<Pose>& newer, const std::optional<Pose>& older,
        float fraction, float snapDistance) -> RenderPose;

    // Returns the 3x4 matrix that moves what was placed with 'from' to where 'to' places it (row vector convention of TransformPoint3x4).
    // note: A rigid move: the rotation that takes the axes of 'from' to the axes of 'to' around the position of 'from', then the new position.
    auto MovePose3x4(const Pose& from, const Pose& to) -> std::array<float, 12>;

    // Returns the row-major 3x4 [R | t] (p' = R p + t, translation at 3, 7 and 11: the layout of the mesh instances) that does 'rowMajor' and
    // then 'move' (a matrix in the layout of TransformPoint3x4 and MovePose3x4, translation at 9, 10 and 11).
    // note: The two layouts differ. Composing them as if they were one sheared every part of a moving object (VW.5e, seen in game).
    auto MoveRowMajor3x4(const std::array<float, 12>& rowMajor, const std::array<float, 12>& move) -> std::array<float, 12>;
}