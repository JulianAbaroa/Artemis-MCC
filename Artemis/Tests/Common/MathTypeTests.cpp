#include <gtest/gtest.h>

#include <cstddef>

import Common.Math.Type;
import std;

// M3a (D175): the vectors of the engine (point2d, point3d, vector4d of the tags and of the datum) are the vectors of Common.Math.
// The generated structures embed them inside #pragma pack(1) structs whose offsets are asserted, so these tests fix the layout
// they rely on: packed floats, no padding, trivially copyable.

namespace
{
    using Common::Math::Type::Vec2;
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Vec4;

#pragma pack(push, 1)
    // Like a generated structure: a byte, then vectors at odd offsets.
    struct PackedLikeTheEngine
    {
        std::uint8_t Flags;
        Vec3 Position;
        Vec4 Rotation;
        Vec2 Range;
    };
#pragma pack(pop)
}

TEST(MathTypeTests, VectorsArePackedFloats)
{
    EXPECT_EQ(sizeof(Vec2), 8u);
    EXPECT_EQ(sizeof(Vec3), 12u);
    EXPECT_EQ(sizeof(Vec4), 16u);
    EXPECT_EQ(alignof(Vec2), alignof(float));
    EXPECT_EQ(alignof(Vec3), alignof(float));
    EXPECT_EQ(alignof(Vec4), alignof(float));
    EXPECT_EQ(offsetof(Vec3, Y), 4u);
    EXPECT_EQ(offsetof(Vec3, Z), 8u);
    EXPECT_EQ(offsetof(Vec4, W), 12u);
}

TEST(MathTypeTests, VectorsAreTriviallyCopyableStandardLayout)
{
    EXPECT_TRUE(std::is_trivially_copyable_v<Vec2>);
    EXPECT_TRUE(std::is_trivially_copyable_v<Vec3>);
    EXPECT_TRUE(std::is_trivially_copyable_v<Vec4>);
    EXPECT_TRUE(std::is_standard_layout_v<Vec2>);
    EXPECT_TRUE(std::is_standard_layout_v<Vec3>);
    EXPECT_TRUE(std::is_standard_layout_v<Vec4>);
}

TEST(MathTypeTests, VectorsKeepTheirPlaceInsideAPackedStruct)
{
    EXPECT_EQ(sizeof(PackedLikeTheEngine), 1u + 12u + 16u + 8u);
    EXPECT_EQ(offsetof(PackedLikeTheEngine, Position), 1u);
    EXPECT_EQ(offsetof(PackedLikeTheEngine, Rotation), 13u);
    EXPECT_EQ(offsetof(PackedLikeTheEngine, Range), 29u);
}

TEST(MathTypeTests, Vec3ReadsTheBytesOfTheEngine)
{
    // Three floats as the engine writes them, at an odd address (inside a packed struct).
    const std::array<float, 3> written{ 1.5f, -2.0f, 3.25f };
    std::array<std::byte, 1 + sizeof(written)> buffer{};
    std::memcpy(buffer.data() + 1, written.data(), sizeof(written));

    Vec3 read{};
    std::memcpy(&read, buffer.data() + 1, sizeof(read));
    EXPECT_EQ(read.X, 1.5f);
    EXPECT_EQ(read.Y, -2.0f);
    EXPECT_EQ(read.Z, 3.25f);
}