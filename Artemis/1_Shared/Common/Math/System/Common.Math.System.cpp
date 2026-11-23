module Common.Math.System;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Pose;
    using Common::Math::Type::PoseSource;
    using Common::Math::Type::RenderPose;
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Vec4;

    using Common::Math::Type::k_WorldUp;

    constexpr Vec3 k_WorldX{ 1.0f, 0.0f, 0.0f };
    constexpr float k_ParallelEpsilon{ 1e-4f };

    auto IsFinitePose(const Pose& pose) -> bool
    {
        return Common::Math::System::IsFinite(pose.Position) && Common::Math::System::IsFinite(pose.Forward) &&
            Common::Math::System::IsFinite(pose.Up);
    }

    // Rows of the rotation of a pose: forward, left and up. Left is up x forward, as the engine places its objects (Z up).
    auto AxesOf(const Pose& pose) -> std::array<Vec3, 3>
    {
        return { pose.Forward, Common::Math::System::Cross(pose.Up, pose.Forward), pose.Up };
    }
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

    auto Rotate(const Vec4& quaternion, const Vec3& v) -> Vec3
    {
        const Vec3 q{ quaternion.X, quaternion.Y, quaternion.Z };
        const Vec3 t{ Scale(Cross(q, v), 2.0f) };
        return Add(Add(v, Scale(t, quaternion.W)), Cross(q, t));
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

    auto IsFinite(const Vec3& v) -> bool
    {
        return std::isfinite(v.X) && std::isfinite(v.Y) && std::isfinite(v.Z);
    }

    auto Distance(const Vec3& a, const Vec3& b) -> float
    {
        return Length(Subtract(a, b));
    }

    auto TransformPoint3x4(const Vec3& point, const float* matrix) -> Vec3
    {
        return {
            point.X * matrix[0] + point.Y * matrix[3] + point.Z * matrix[6] + matrix[9],
            point.X * matrix[1] + point.Y * matrix[4] + point.Z * matrix[7] + matrix[10],
            point.X * matrix[2] + point.Y * matrix[5] + point.Z * matrix[8] + matrix[11] };
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
        outForward = Normalize(forwardIn, normalizeFallback);

        Vec3 right = Cross(outForward, k_WorldUp);

        if (Dot(right, right) < k_ParallelEpsilon)
        {
            right = Cross(outForward, k_WorldX);
        }

        outRight = Normalize(right, normalizeFallback);
        outUp = Cross(outRight, outForward);
    }

    auto BlendPose(const Pose& a, const Pose& b, float t) -> Pose
    {
        Pose result{};
        result.Position = Add(a.Position, Scale(Subtract(b.Position, a.Position), t));

        const Vec3 forward{ Add(a.Forward, Scale(Subtract(b.Forward, a.Forward), t)) };
        const Vec3 up{ Add(a.Up, Scale(Subtract(b.Up, a.Up), t)) };

        // note: The blend of two unit vectors is not a unit vector, and the two blended axes are no longer at right angles.
        result.Forward = Normalize(forward, a.Forward);
        result.Up = Normalize(Subtract(up, Scale(result.Forward, Dot(up, result.Forward))), a.Up);
        return result;
    }

    auto ResolveRenderPose(const Pose& previous, const std::optional<Pose>& newer, const std::optional<Pose>& older,
        float fraction, float snapDistance) -> RenderPose
    {
        const RenderPose unchanged{ .Value = previous, .Source = PoseSource::None };

        if (!std::isfinite(fraction) || fraction < 0.0f || fraction > 1.0f || !IsFinitePose(previous)) return unchanged;

        if (newer)
        {
            if (!IsFinitePose(*newer) || Distance(previous.Position, newer->Position) > snapDistance) return unchanged;
            return RenderPose{ .Value = BlendPose(previous, *newer, fraction), .Source = PoseSource::Exact };
        }

        if (older)
        {
            if (!IsFinitePose(*older) || Distance(older->Position, previous.Position) > snapDistance) return unchanged;
            return RenderPose{ .Value = BlendPose(*older, previous, 1.0f + fraction), .Source = PoseSource::Extrapolated };
        }

        return unchanged;
    }

    auto MovePose3x4(const Pose& from, const Pose& to) -> std::array<float, 12>
    {
        const std::array<Vec3, 3> source{ AxesOf(from) };
        const std::array<Vec3, 3> target{ AxesOf(to) };

        const auto component = [](const Vec3& v, std::size_t index) -> float
        {
            return index == 0 ? v.X : index == 1 ? v.Y : v.Z;
        };

        std::array<float, 12> matrix{};

        // note: A point has the coordinates c(i) along the axes of 'from' and goes to the same c(i) along the axes of 'to': row j is the sum over i of source(i)[j] * target(i).
        for (std::size_t row{ 0 }; row < 3; ++row)
        {
            for (std::size_t column{ 0 }; column < 3; ++column)
            {
                float sum{ 0.0f };
                for (std::size_t axis{ 0 }; axis < 3; ++axis) sum += component(source[axis], row) * component(target[axis], column);
                matrix[row * 3 + column] = sum;
            }
        }

        // note: The translation is still zero here, so this is the position of 'from' turned by the matrix.
        const Vec3 moved{ TransformPoint3x4(from.Position, matrix.data()) };
        matrix[9] = to.Position.X - moved.X;
        matrix[10] = to.Position.Y - moved.Y;
        matrix[11] = to.Position.Z - moved.Z;
        return matrix;
    }

    auto MoveRowMajor3x4(const std::array<float, 12>& rowMajor, const std::array<float, 12>& move) -> std::array<float, 12>
    {
        std::array<float, 12> result{};

        // note: Output c of 'move' is the sum over i of input(i) * move[i * 3 + c] plus move[9 + c]. Input(i) is row i of 'rowMajor'.
        for (std::size_t c{ 0 }; c < 3; ++c)
        {
            for (std::size_t k{ 0 }; k < 4; ++k)
            {
                float sum{ 0.0f };
                for (std::size_t i{ 0 }; i < 3; ++i) sum += move[i * 3 + c] * rowMajor[i * 4 + k];
                result[c * 4 + k] = sum;
            }

            result[c * 4 + 3] += move[9 + c];
        }

        return result;
    }
}