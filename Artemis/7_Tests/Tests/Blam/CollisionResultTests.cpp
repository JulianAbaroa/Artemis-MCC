#include <gtest/gtest.h>

import Common.Engine.Type;
import Blam.Layout.Collision.Type;
import std;

// What the raycast oracle (R3, D272) reads from the engine: the flags it passes to collision_test_vector and the 0x70 bytes it gets back.

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using namespace Blam::Layout::Collision::Type;
    using Blam::Layout::Collision::Type::Result::CollisionResult;
    using Blam::Layout::Collision::Type::Result::Decode;
    using Blam::Layout::Collision::Type::Result::ResultKind;

    template <typename T>
    auto Put(std::array<std::uint8_t, Constant::k_ResultSize>& bytes, std::size_t offset, T value) -> void
    {
        std::memcpy(bytes.data() + offset, &value, sizeof(T));
    }
}

TEST(CollisionFlags, StaticTestsStructureAndInstancesWithEverySurface)
{
    EXPECT_NE(Flag::k_Static & Flag::k_Structure, 0u);
    EXPECT_NE(Flag::k_Static & Flag::k_InstancedGeometry, 0u);
    EXPECT_NE(Flag::k_Static & Flag::k_KeepObjectClass1000, 0u);
    EXPECT_EQ(Flag::k_Static & Flag::k_Objects, 0u);
    EXPECT_EQ(Flag::k_Static & Flag::k_NarrowingBits, 0u);
}

TEST(CollisionFlags, StaticValueIsBitsZeroThreeAndTwentyThree)
{
    EXPECT_EQ(Flag::k_Static, (1ull << 0) | (1ull << 3) | (1ull << 23));
}

TEST(CollisionFlags, WithObjectsAddsBitsThirtyTwoAndFortySeven)
{
    EXPECT_EQ(Flag::k_StaticAndObjects, Flag::k_Static | (1ull << 32) | (1ull << 47));
    EXPECT_EQ(Flag::k_StaticAndObjects & Flag::k_NarrowingBits, 0u);
}

TEST(CollisionResultDecode, NothingHitKeepsTheEngineInitialValues)
{
    std::array<std::uint8_t, Constant::k_ResultSize> bytes{};
    Put<float>(bytes, Offset::k_Fraction, 1.0f);
    Put<std::uint32_t>(bytes, Offset::k_ObjectHandle, k_NoHandle);
    Put<std::uint16_t>(bytes, Offset::k_Material, 0xFFFF);

    const CollisionResult result{ Decode(false, bytes) };

    EXPECT_FALSE(result.IsHit);
    EXPECT_EQ(result.Kind, ResultKind::None);
    EXPECT_FLOAT_EQ(result.Fraction, 1.0f);
    EXPECT_EQ(result.Material, -1);
    EXPECT_EQ(result.ObjectHandle, k_NoHandle);
}

TEST(CollisionResultDecode, StructureHitReadsEveryField)
{
    std::array<std::uint8_t, Constant::k_ResultSize> bytes{};
    Put<std::uint32_t>(bytes, Offset::k_ResultKind, 1);
    Put<float>(bytes, Offset::k_Fraction, 0.25f);
    Put<float>(bytes, Offset::k_Point, 10.0f);
    Put<float>(bytes, Offset::k_Point + 4, -20.0f);
    Put<float>(bytes, Offset::k_Point + 8, 30.5f);
    Put<std::int16_t>(bytes, Offset::k_Material, 42);
    Put<std::uint8_t>(bytes, Offset::k_IsBackFace, 1);
    Put<std::uint8_t>(bytes, Offset::k_SurfaceFlags, 0x0A);
    Put<std::uint8_t>(bytes, Offset::k_SurfaceByteA, 0x11);
    Put<std::uint8_t>(bytes, Offset::k_SurfaceByteB, 0x22);

    const CollisionResult result{ Decode(true, bytes) };

    EXPECT_TRUE(result.IsHit);
    EXPECT_EQ(result.Kind, ResultKind::Structure);
    EXPECT_FLOAT_EQ(result.Fraction, 0.25f);
    EXPECT_FLOAT_EQ(result.Point[0], 10.0f);
    EXPECT_FLOAT_EQ(result.Point[1], -20.0f);
    EXPECT_FLOAT_EQ(result.Point[2], 30.5f);
    EXPECT_EQ(result.Material, 42);
    EXPECT_TRUE(result.IsBackFace);
    EXPECT_EQ(result.SurfaceFlags, 0x0A);
    EXPECT_EQ(result.SurfaceByteA, 0x11);
    EXPECT_EQ(result.SurfaceByteB, 0x22);
}

TEST(CollisionResultDecode, ObjectHitReadsTheHandle)
{
    std::array<std::uint8_t, Constant::k_ResultSize> bytes{};
    Put<std::uint32_t>(bytes, Offset::k_ResultKind, 4);
    Put<std::uint32_t>(bytes, Offset::k_ObjectHandle, 0xABCD0012u);

    const CollisionResult result{ Decode(true, bytes) };

    EXPECT_EQ(result.Kind, ResultKind::Object);
    EXPECT_EQ(result.ObjectHandle, 0xABCD0012u);
}

TEST(CollisionResultDecode, FieldsStayInsideTheResult)
{
    EXPECT_LT(Offset::k_SurfaceByteB, Constant::k_ResultSize);
    EXPECT_LT(Offset::k_ObjectHandle + sizeof(std::uint32_t), Constant::k_ResultSize + 1);
}