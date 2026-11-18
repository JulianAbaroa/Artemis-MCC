#include <gtest/gtest.h>

import Engine.Base.Memory.Type;
import Engine.Base.Memory.System;
import std;

// The validation of the game camera read from s_render_projection (VW.5a, D323): a buffer of 0x30 floats, no game needed.

namespace
{
    using Engine::Base::Memory::System::GameCameraService;
    using Engine::Base::Memory::Type::GameCamera::k_ProjectionFloatCount;

    using Floats = std::array<float, k_ProjectionFloatCount>;

    // A camera at (1, 2, 3) looking along +X with +Z up: right = -Y, forward = X (the view_to_world block holds minus forward).
    auto MakeFloats() -> Floats
    {
        Floats floats{};
        floats[0x0D] = 1.0f;
        floats[0x0E] = 0.0f; floats[0x0F] = -1.0f; floats[0x10] = 0.0f;
        floats[0x11] = 0.0f; floats[0x12] = 0.0f; floats[0x13] = 1.0f;
        floats[0x14] = -1.0f; floats[0x15] = 0.0f; floats[0x16] = 0.0f;
        floats[0x17] = 1.0f; floats[0x18] = 2.0f; floats[0x19] = 3.0f;
        floats[0x1E] = 0.75f;
        floats[0x23] = 1.0f;
        floats[0x26] = 0.01f;
        floats[0x27] = -0.02f;
        floats[0x29] = -1.0f;
        return floats;
    }
}

TEST(GameCameraTests, AValidBufferGivesTheCamera)
{
    const auto camera{ GameCameraService::Parse(MakeFloats()) };

    ASSERT_TRUE(camera.has_value());
    EXPECT_FLOAT_EQ(camera->Position.X, 1.0f);
    EXPECT_FLOAT_EQ(camera->Position.Y, 2.0f);
    EXPECT_FLOAT_EQ(camera->Position.Z, 3.0f);
    EXPECT_FLOAT_EQ(camera->Forward.X, 1.0f);
    EXPECT_FLOAT_EQ(camera->Right.Y, -1.0f);
    EXPECT_FLOAT_EQ(camera->Up.Z, 1.0f);
    EXPECT_FLOAT_EQ(camera->M00, 0.75f);
    EXPECT_FLOAT_EQ(camera->M11, 1.0f);
    EXPECT_FLOAT_EQ(camera->M20, 0.01f);
    EXPECT_FLOAT_EQ(camera->M21, -0.02f);
}

TEST(GameCameraTests, ANotFiniteFloatGivesNothing)
{
    for (const std::size_t index : { std::size_t{ 0x0E }, std::size_t{ 0x19 }, std::size_t{ 0x1E }, std::size_t{ 0x27 } })
    {
        Floats floats{ MakeFloats() };
        floats[index] = std::numeric_limits<float>::quiet_NaN();
        EXPECT_FALSE(GameCameraService::Parse(floats).has_value()) << index;

        floats[index] = std::numeric_limits<float>::infinity();
        EXPECT_FALSE(GameCameraService::Parse(floats).has_value()) << index;
    }
}

TEST(GameCameraTests, AxesThatAreNotOrthonormalGiveNothing)
{
    Floats longAxis{ MakeFloats() };
    longAxis[0x0F] = -1.01f;
    EXPECT_FALSE(GameCameraService::Parse(longAxis).has_value());

    Floats skewed{ MakeFloats() };
    skewed[0x11] = 0.01f;
    EXPECT_FALSE(GameCameraService::Parse(skewed).has_value());

    const Floats zero{};
    EXPECT_FALSE(GameCameraService::Parse(zero).has_value());
}

TEST(GameCameraTests, AProjectionThatIsNotPositiveGivesNothing)
{
    Floats noWidth{ MakeFloats() };
    noWidth[0x1E] = 0.0f;
    EXPECT_FALSE(GameCameraService::Parse(noWidth).has_value());

    Floats negativeHeight{ MakeFloats() };
    negativeHeight[0x23] = -1.0f;
    EXPECT_FALSE(GameCameraService::Parse(negativeHeight).has_value());
}