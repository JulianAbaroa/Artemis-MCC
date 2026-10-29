#include <gtest/gtest.h>

import Common.Math.Type;
import Common.ZoneShape.Type;
import Common.ZoneShape.System;
import std;

// B4h H.2: the containment test of a point in a zone follows the convention the viewer draws: the shape is measured from the
// WorldOrigin of the object, along its Forward, its Right (Forward x Up) and its Up.

namespace
{
    using Common::Math::Type::Vec3;
    using Common::ZoneShape::Type::ZoneShape;
    using Common::ZoneShape::Type::ZoneShapeKind;
    using Common::ZoneShape::System::Contains;

    const Vec3 k_Origin{ 10.0f, 20.0f, 30.0f };
    const Vec3 k_Forward{ 0.0f, 1.0f, 0.0f };
    const Vec3 k_Up{ 0.0f, 0.0f, 1.0f };

    const ZoneShape k_Cylinder{ .Radius = 2.0f, .Top = 3.0f, .Bottom = 1.0f, .Kind = ZoneShapeKind::Cylinder };
    const ZoneShape k_Box{ .Radius = 4.0f, .Length = 6.0f, .Top = 3.0f, .Bottom = 1.0f, .Kind = ZoneShapeKind::Box };

    auto At(float right, float ahead, float up) -> Vec3
    {
        // Right = Forward x Up = (1, 0, 0) for this pose.
        return Vec3{ k_Origin.X + right, k_Origin.Y + ahead, k_Origin.Z + up };
    }
}

TEST(ZoneShapeTests, CylinderIsRoundAroundTheOriginAndGoesFromBottomToTop)
{
    EXPECT_TRUE(Contains(k_Cylinder, k_Origin, k_Forward, k_Up, k_Origin));
    EXPECT_TRUE(Contains(k_Cylinder, k_Origin, k_Forward, k_Up, At(1.4f, 1.4f, 0.0f)));
    EXPECT_FALSE(Contains(k_Cylinder, k_Origin, k_Forward, k_Up, At(1.6f, 1.6f, 0.0f)));
    EXPECT_TRUE(Contains(k_Cylinder, k_Origin, k_Forward, k_Up, At(0.0f, 0.0f, 2.9f)));
    EXPECT_FALSE(Contains(k_Cylinder, k_Origin, k_Forward, k_Up, At(0.0f, 0.0f, 3.1f)));
    EXPECT_TRUE(Contains(k_Cylinder, k_Origin, k_Forward, k_Up, At(0.0f, 0.0f, -0.9f)));
    EXPECT_FALSE(Contains(k_Cylinder, k_Origin, k_Forward, k_Up, At(0.0f, 0.0f, -1.1f)));
}

TEST(ZoneShapeTests, BoxHasTheWidthAlongRightAndTheLengthAlongForward)
{
    EXPECT_TRUE(Contains(k_Box, k_Origin, k_Forward, k_Up, At(1.9f, 2.9f, 0.0f)));
    EXPECT_FALSE(Contains(k_Box, k_Origin, k_Forward, k_Up, At(2.1f, 0.0f, 0.0f)));
    EXPECT_FALSE(Contains(k_Box, k_Origin, k_Forward, k_Up, At(0.0f, 3.1f, 0.0f)));
    EXPECT_TRUE(Contains(k_Box, k_Origin, k_Forward, k_Up, At(-1.9f, -2.9f, 2.9f)));
    EXPECT_FALSE(Contains(k_Box, k_Origin, k_Forward, k_Up, At(0.0f, 0.0f, 3.1f)));
}

TEST(ZoneShapeTests, TheShapeTurnsWithTheForwardOfTheObject)
{
    // Forward along +X: Right = Forward x Up = (0, -1, 0), so the length of the box is now along X.
    const Vec3 forward{ 1.0f, 0.0f, 0.0f };
    EXPECT_TRUE(Contains(k_Box, k_Origin, forward, k_Up, At(2.9f, 1.9f, 0.0f)));
    EXPECT_FALSE(Contains(k_Box, k_Origin, forward, k_Up, At(3.1f, 0.0f, 0.0f)));
    EXPECT_FALSE(Contains(k_Box, k_Origin, forward, k_Up, At(0.0f, 2.1f, 0.0f)));
}

TEST(ZoneShapeTests, AZoneWithoutShapeContainsNothing)
{
    EXPECT_FALSE(Contains(ZoneShape{}, k_Origin, k_Forward, k_Up, k_Origin));
}