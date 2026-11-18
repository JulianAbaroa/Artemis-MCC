#include <gtest/gtest.h>

import Common.Math.System;
import Common.Math.Type;
import std;

namespace
{
    using Common::Math::System::BlendPose;
    using Common::Math::System::Compose3x4;
    using Common::Math::System::Cross;
    using Common::Math::System::Dot;
    using Common::Math::System::Length;
    using Common::Math::System::MovePose3x4;
    using Common::Math::System::ResolveRenderPose;
    using Common::Math::System::TransformPoint3x4;
    using Common::Math::Type::Pose;
    using Common::Math::Type::PoseSource;
    using Common::Math::Type::Vec3;

    constexpr float k_Snap{ 2.0f };
    constexpr float k_Near{ 1e-4f };

    // Pose at the position, facing the heading in radians around Z.
    auto PoseAt(float x, float y, float z, float heading = 0.0f) -> Pose
    {
        return Pose{ .Position = { x, y, z }, .Forward = { std::cos(heading), std::sin(heading), 0.0f }, .Up = { 0.0f, 0.0f, 1.0f } };
    }

    auto ExpectNear(const Vec3& actual, const Vec3& expected, float tolerance = k_Near) -> void
    {
        EXPECT_NEAR(actual.X, expected.X, tolerance);
        EXPECT_NEAR(actual.Y, expected.Y, tolerance);
        EXPECT_NEAR(actual.Z, expected.Z, tolerance);
    }

    auto ExpectOrthonormal(const Pose& pose) -> void
    {
        EXPECT_NEAR(Length(pose.Forward), 1.0f, k_Near);
        EXPECT_NEAR(Length(pose.Up), 1.0f, k_Near);
        EXPECT_NEAR(Dot(pose.Forward, pose.Up), 0.0f, k_Near);
    }
}

TEST(PoseInterpolationTests, BlendGivesTheEndsAtZeroAndOne)
{
    const Pose a{ PoseAt(1.0f, 2.0f, 3.0f, 0.0f) };
    const Pose b{ PoseAt(5.0f, -2.0f, 4.0f, 1.0f) };

    const Pose atZero{ BlendPose(a, b, 0.0f) };
    ExpectNear(atZero.Position, a.Position);
    ExpectNear(atZero.Forward, a.Forward);

    const Pose atOne{ BlendPose(a, b, 1.0f) };
    ExpectNear(atOne.Position, b.Position);
    ExpectNear(atOne.Forward, b.Forward);
    ExpectNear(atOne.Up, b.Up);
}

TEST(PoseInterpolationTests, BlendTurnsHalfWayAndKeepsTheAxesUnit)
{
    const Pose a{ PoseAt(0.0f, 0.0f, 0.0f, 0.0f) };
    const Pose b{ PoseAt(2.0f, 0.0f, 0.0f, std::numbers::pi_v<float> / 2.0f) };

    const Pose half{ BlendPose(a, b, 0.5f) };
    ExpectNear(half.Position, Vec3{ 1.0f, 0.0f, 0.0f });
    ExpectNear(half.Forward, Vec3{ std::sqrt(0.5f), std::sqrt(0.5f), 0.0f });
    ExpectOrthonormal(half);
}

TEST(PoseInterpolationTests, BlendKeepsTheAxesOfAWhenTheyCollapse)
{
    const Pose a{ PoseAt(0.0f, 0.0f, 0.0f, 0.0f) };
    const Pose b{ PoseAt(0.0f, 0.0f, 0.0f, std::numbers::pi_v<float>) };

    const Pose half{ BlendPose(a, b, 0.5f) };
    ExpectNear(half.Forward, a.Forward, 1e-3f);
    ExpectNear(half.Up, a.Up);
}

TEST(PoseInterpolationTests, ExactIsBetweenTheTwoTicks)
{
    const Pose previous{ PoseAt(0.0f, 0.0f, 0.0f) };
    const Pose newer{ PoseAt(1.0f, 0.0f, 0.0f) };

    const auto atZero{ ResolveRenderPose(previous, newer, std::nullopt, 0.0f, k_Snap) };
    EXPECT_EQ(atZero.Source, PoseSource::Exact);
    ExpectNear(atZero.Value.Position, previous.Position);

    const auto atOne{ ResolveRenderPose(previous, newer, std::nullopt, 1.0f, k_Snap) };
    EXPECT_EQ(atOne.Source, PoseSource::Exact);
    ExpectNear(atOne.Value.Position, newer.Position);

    const auto quarter{ ResolveRenderPose(previous, newer, std::nullopt, 0.25f, k_Snap) };
    ExpectNear(quarter.Value.Position, Vec3{ 0.25f, 0.0f, 0.0f });
}

TEST(PoseInterpolationTests, ExactWinsOverTheOlderPose)
{
    const auto result{ ResolveRenderPose(PoseAt(1.0f, 0.0f, 0.0f), PoseAt(2.0f, 0.0f, 0.0f), PoseAt(0.0f, 0.0f, 0.0f), 0.5f, k_Snap) };
    EXPECT_EQ(result.Source, PoseSource::Exact);
    ExpectNear(result.Value.Position, Vec3{ 1.5f, 0.0f, 0.0f });
}

TEST(PoseInterpolationTests, ExtrapolatesAtConstantVelocityFromTheTwoLastTicks)
{
    const Pose older{ PoseAt(0.0f, 0.0f, 0.0f, 0.0f) };
    const Pose previous{ PoseAt(1.0f, 0.0f, 0.0f, 0.2f) };

    const auto atZero{ ResolveRenderPose(previous, std::nullopt, older, 0.0f, k_Snap) };
    EXPECT_EQ(atZero.Source, PoseSource::Extrapolated);
    ExpectNear(atZero.Value.Position, previous.Position);
    ExpectNear(atZero.Value.Forward, previous.Forward);

    const auto half{ ResolveRenderPose(previous, std::nullopt, older, 0.5f, k_Snap) };
    EXPECT_EQ(half.Source, PoseSource::Extrapolated);
    ExpectNear(half.Value.Position, Vec3{ 1.5f, 0.0f, 0.0f });
    ExpectOrthonormal(half.Value);

    const auto one{ ResolveRenderPose(previous, std::nullopt, older, 1.0f, k_Snap) };
    ExpectNear(one.Value.Position, Vec3{ 2.0f, 0.0f, 0.0f });
}

TEST(PoseInterpolationTests, WithoutNeighboursTheShownPoseIsKept)
{
    const Pose previous{ PoseAt(3.0f, 4.0f, 5.0f, 0.7f) };

    const auto result{ ResolveRenderPose(previous, std::nullopt, std::nullopt, 0.5f, k_Snap) };
    EXPECT_EQ(result.Source, PoseSource::None);
    ExpectNear(result.Value.Position, previous.Position, 0.0f);
    ExpectNear(result.Value.Forward, previous.Forward, 0.0f);
}

TEST(PoseInterpolationTests, ALongStepIsNotBlended)
{
    const Pose previous{ PoseAt(0.0f, 0.0f, 0.0f) };
    const Pose far{ PoseAt(10.0f, 0.0f, 0.0f) };

    const auto exact{ ResolveRenderPose(previous, far, std::nullopt, 0.5f, k_Snap) };
    EXPECT_EQ(exact.Source, PoseSource::None);
    ExpectNear(exact.Value.Position, previous.Position, 0.0f);

    const auto extrapolated{ ResolveRenderPose(previous, std::nullopt, far, 0.5f, k_Snap) };
    EXPECT_EQ(extrapolated.Source, PoseSource::None);
    ExpectNear(extrapolated.Value.Position, previous.Position, 0.0f);

    // A step just inside the limit still blends.
    const auto inside{ ResolveRenderPose(previous, PoseAt(2.0f, 0.0f, 0.0f), std::nullopt, 0.5f, k_Snap) };
    EXPECT_EQ(inside.Source, PoseSource::Exact);
}

TEST(PoseInterpolationTests, AFractionOutsideZeroToOneIsNotUsed)
{
    const Pose previous{ PoseAt(0.0f, 0.0f, 0.0f) };
    const Pose newer{ PoseAt(1.0f, 0.0f, 0.0f) };

    EXPECT_EQ(ResolveRenderPose(previous, newer, std::nullopt, -0.1f, k_Snap).Source, PoseSource::None);
    EXPECT_EQ(ResolveRenderPose(previous, newer, std::nullopt, 1.1f, k_Snap).Source, PoseSource::None);
    EXPECT_EQ(ResolveRenderPose(previous, newer, std::nullopt, std::numeric_limits<float>::quiet_NaN(), k_Snap).Source, PoseSource::None);
}

TEST(PoseInterpolationTests, APoseThatIsNotFiniteIsNotUsed)
{
    const Pose previous{ PoseAt(0.0f, 0.0f, 0.0f) };
    Pose broken{ PoseAt(1.0f, 0.0f, 0.0f) };
    broken.Position.X = std::numeric_limits<float>::quiet_NaN();

    const auto result{ ResolveRenderPose(previous, broken, std::nullopt, 0.5f, k_Snap) };
    EXPECT_EQ(result.Source, PoseSource::None);
    ExpectNear(result.Value.Position, previous.Position, 0.0f);
}

TEST(PoseInterpolationTests, MoveSendsThePointsOfOnePoseToTheOther)
{
    const Pose from{ PoseAt(1.0f, 2.0f, 3.0f, 0.3f) };
    const Pose to{ PoseAt(-4.0f, 0.5f, 3.5f, 1.2f) };

    const auto matrix{ MovePose3x4(from, to) };

    // The origin of the pose goes to the new origin.
    ExpectNear(TransformPoint3x4(from.Position, matrix.data()), to.Position);

    // A point one unit ahead of the pose, along its forward, is one unit ahead of the new pose.
    const Vec3 ahead{ from.Position.X + from.Forward.X, from.Position.Y + from.Forward.Y, from.Position.Z + from.Forward.Z };
    const Vec3 expected{ to.Position.X + to.Forward.X, to.Position.Y + to.Forward.Y, to.Position.Z + to.Forward.Z };
    ExpectNear(TransformPoint3x4(ahead, matrix.data()), expected);

    // The same for the left side and for up.
    const Vec3 left{ Cross(from.Up, from.Forward) };
    const Vec3 leftTo{ Cross(to.Up, to.Forward) };
    ExpectNear(TransformPoint3x4(Vec3{ from.Position.X + left.X, from.Position.Y + left.Y, from.Position.Z + left.Z }, matrix.data()),
        Vec3{ to.Position.X + leftTo.X, to.Position.Y + leftTo.Y, to.Position.Z + leftTo.Z });
    ExpectNear(TransformPoint3x4(Vec3{ from.Position.X, from.Position.Y, from.Position.Z + 1.0f }, matrix.data()),
        Vec3{ to.Position.X + to.Up.X, to.Position.Y + to.Up.Y, to.Position.Z + to.Up.Z });
}

TEST(PoseInterpolationTests, MoveBetweenEqualPosesChangesNothing)
{
    const Pose pose{ PoseAt(7.0f, -3.0f, 1.0f, 2.0f) };
    const auto matrix{ MovePose3x4(pose, pose) };

    const Vec3 point{ 9.0f, 4.0f, -2.0f };
    ExpectNear(TransformPoint3x4(point, matrix.data()), point);
}

TEST(PoseInterpolationTests, ComposeDoesTheFirstMoveAndThenTheSecond)
{
    const Pose a{ PoseAt(0.0f, 0.0f, 0.0f, 0.0f) };
    const Pose b{ PoseAt(3.0f, 1.0f, 0.0f, 0.8f) };
    const Pose c{ PoseAt(-2.0f, 5.0f, 1.0f, 2.0f) };

    const auto first{ MovePose3x4(a, b) };
    const auto second{ MovePose3x4(b, c) };
    const auto both{ Compose3x4(first, second) };

    const Vec3 point{ 0.7f, -1.3f, 2.2f };
    const Vec3 stepwise{ TransformPoint3x4(TransformPoint3x4(point, first.data()), second.data()) };
    ExpectNear(TransformPoint3x4(point, both.data()), stepwise, 1e-3f);
}