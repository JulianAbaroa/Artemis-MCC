#include <gtest/gtest.h>

import Common.Math.Type;
import Engine.Base.Memory.Type;
import Engine.Base.Memory.State;
import Engine.Base.Memory.System;
import std;

// The validation of the game camera read from s_render_projection (VW.5a, D323): a buffer of 0x30 floats, no game needed.

namespace
{
    using Engine::Base::Memory::System::GameCameraService;
    using Engine::Base::Memory::Type::GameCamera::k_ProjectionFloatCount;

    using Common::Math::Type::Vec3;
    using Engine::Base::Memory::State::CameraOverrideStore;
    using Engine::Base::Memory::Type::ObserverCamera::CameraOverride;
    using Engine::Base::Memory::Type::Store::ReaderStallClock;
    using Engine::Base::Memory::Type::Store::k_ReaderStallNanoseconds;

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

// The override of the game camera (VW.5b): the validation of a requested pose and the seqlock that carries it to the game thread.

namespace
{
    // At (10, 20, 30) looking along +X with +Z up.
    auto MakePose() -> CameraOverride
    {
        return CameraOverride{ .Eye = Vec3{ 10.0f, 20.0f, 30.0f }, .Forward = Vec3{ 1.0f, 0.0f, 0.0f }, .Up = Vec3{ 0.0f, 0.0f, 1.0f } };
    }
}

TEST(GameCameraTests, AnOrthonormalPoseInsideTheLimitIsValid)
{
    EXPECT_TRUE(GameCameraService::IsValidOverride(MakePose()));

    // A tilted pair is still valid when it is orthonormal.
    CameraOverride tilted{ MakePose() };
    tilted.Forward = Vec3{ 0.6f, 0.8f, 0.0f };
    tilted.Up = Vec3{ 0.0f, 0.0f, 1.0f };
    EXPECT_TRUE(GameCameraService::IsValidOverride(tilted));
}

TEST(GameCameraTests, APoseThatIsNotFiniteIsNotValid)
{
    const float nan{ std::numeric_limits<float>::quiet_NaN() };
    const float inf{ std::numeric_limits<float>::infinity() };

    CameraOverride eye{ MakePose() };
    eye.Eye.Y = nan;
    EXPECT_FALSE(GameCameraService::IsValidOverride(eye));

    CameraOverride forward{ MakePose() };
    forward.Forward.X = inf;
    EXPECT_FALSE(GameCameraService::IsValidOverride(forward));

    CameraOverride up{ MakePose() };
    up.Up.Z = nan;
    EXPECT_FALSE(GameCameraService::IsValidOverride(up));
}

TEST(GameCameraTests, AnAxisThatIsNotUnitOrPerpendicularIsNotValid)
{
    CameraOverride longForward{ MakePose() };
    longForward.Forward = Vec3{ 2.0f, 0.0f, 0.0f };
    EXPECT_FALSE(GameCameraService::IsValidOverride(longForward));

    CameraOverride zeroUp{ MakePose() };
    zeroUp.Up = Vec3{ 0.0f, 0.0f, 0.0f };
    EXPECT_FALSE(GameCameraService::IsValidOverride(zeroUp));

    CameraOverride skewed{ MakePose() };
    skewed.Up = Vec3{ 0.6f, 0.0f, 0.8f };
    EXPECT_FALSE(GameCameraService::IsValidOverride(skewed));
}

TEST(GameCameraTests, AnEyeFarFromTheOriginIsNotValid)
{
    CameraOverride far{ MakePose() };
    far.Eye = Vec3{ 50000.0f, 0.0f, 0.0f };
    EXPECT_FALSE(GameCameraService::IsValidOverride(far));

    far.Eye = Vec3{ 40000.0f, 40000.0f, 0.0f };
    EXPECT_FALSE(GameCameraService::IsValidOverride(far));

    far.Eye = Vec3{ 49000.0f, 0.0f, 0.0f };
    EXPECT_TRUE(GameCameraService::IsValidOverride(far));
}

TEST(GameCameraTests, TheOverrideStoreGivesWhatWasPublishedUntilItIsCleared)
{
    CameraOverrideStore store{};
    EXPECT_FALSE(store.IsActive());
    EXPECT_FALSE(store.Read(std::chrono::seconds{ 1 }).has_value());

    store.Publish(MakePose());
    EXPECT_TRUE(store.IsActive());

    const std::optional<CameraOverride> pose{ store.Read(std::chrono::seconds{ 1 }) };
    ASSERT_TRUE(pose.has_value());
    EXPECT_EQ(pose->Eye.X, 10.0f);
    EXPECT_EQ(pose->Eye.Z, 30.0f);
    EXPECT_EQ(pose->Forward.X, 1.0f);
    EXPECT_EQ(pose->Up.Z, 1.0f);

    store.Clear();
    EXPECT_FALSE(store.IsActive());
    EXPECT_FALSE(store.Read(std::chrono::seconds{ 1 }).has_value());
}

TEST(GameCameraTests, AStalePoseIsTakenAsNone)
{
    CameraOverrideStore store{};
    store.Publish(MakePose());

    std::this_thread::sleep_for(std::chrono::milliseconds{ 20 });

    EXPECT_FALSE(store.Read(std::chrono::milliseconds{ 5 }).has_value());
    EXPECT_TRUE(store.Read(std::chrono::seconds{ 5 }).has_value());

    // A new pose is fresh again.
    store.Publish(MakePose());
    EXPECT_TRUE(store.Read(std::chrono::milliseconds{ 5 }).has_value());
}

TEST(GameCameraTests, AStallOfTheReaderDoesNotAgeThePose)
{
    // The game thread stood still (D379): the Viewer could not publish, so the last pose is still the current one.
    CameraOverrideStore store{};
    store.Publish(MakePose());
    EXPECT_TRUE(store.Read(std::chrono::milliseconds{ 100 }).has_value());

    std::this_thread::sleep_for(std::chrono::milliseconds{ 250 });

    EXPECT_TRUE(store.Read(std::chrono::milliseconds{ 100 }).has_value());
}

TEST(GameCameraTests, APoseAgesOutWhileTheReaderKeepsRunning)
{
    // The writer stopped and the game thread keeps reading every frame: the pose is none after the max age.
    CameraOverrideStore store{};
    store.Publish(MakePose());

    bool isFresh{ true };
    for (int frame{ 0 }; frame < 15 && isFresh; ++frame)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{ 20 });
        isFresh = store.Read(std::chrono::milliseconds{ 100 }).has_value();
    }

    EXPECT_FALSE(isFresh);
}

TEST(GameCameraTests, TheReaderStallClockCountsOnlyGapsLongerThanAFrame)
{
    constexpr std::int64_t start{ 1'000'000'000 };
    constexpr std::int64_t frame{ 16'000'000 };
    constexpr std::int64_t stall{ k_ReaderStallNanoseconds + 1 };

    ReaderStallClock clock{};
    EXPECT_EQ(clock.Read(start), 0);
    EXPECT_EQ(clock.Read(start + frame), 0);
    EXPECT_EQ(clock.Read(start + frame + stall), stall);
    EXPECT_EQ(clock.Read(start + 2 * frame + stall), stall);

    // A new value forgets the stalls.
    clock.Reset();
    EXPECT_EQ(clock.Read(start + 3 * frame + stall), 0);
}

TEST(GameCameraTests, TheOverrideStoreCountsFaults)
{
    CameraOverrideStore store{};
    EXPECT_EQ(store.GetFaultCount(), 0u);

    store.CountFault();
    store.CountFault();
    EXPECT_EQ(store.GetFaultCount(), 2u);
}

TEST(GameCameraTests, SettingAnInvalidOverrideClearsTheOneThatWasSet)
{
    CameraOverrideStore store{};
    Engine::Base::Memory::System::MemoryReaderService reader{};
    GameCameraService service{ reader, store };

    EXPECT_TRUE(service.SetOverride(Vec3{ 1.0f, 2.0f, 3.0f }, Vec3{ 1.0f, 0.0f, 0.0f }, Vec3{ 0.0f, 0.0f, 1.0f }));
    EXPECT_TRUE(service.IsOverrideActive());

    EXPECT_FALSE(service.SetOverride(Vec3{ 1.0f, 2.0f, 3.0f }, Vec3{ 2.0f, 0.0f, 0.0f }, Vec3{ 0.0f, 0.0f, 1.0f }));
    EXPECT_FALSE(service.IsOverrideActive());

    EXPECT_TRUE(service.SetOverride(Vec3{ 1.0f, 2.0f, 3.0f }, Vec3{ 1.0f, 0.0f, 0.0f }, Vec3{ 0.0f, 0.0f, 1.0f }));
    service.ClearOverride();
    EXPECT_FALSE(service.IsOverrideActive());
}

TEST(GameCameraTests, TheOverrideStoreNeverGivesATornPose)
{
    CameraOverrideStore store{};
    std::atomic<bool> isDone{ false };

    // The writer alternates two poses whose nine floats are all the same value each, so a mix of two writes shows as different floats.
    std::thread writer{ [&] {
        for (int i{ 0 }; i < 200000; ++i)
        {
            const float value{ (i % 2 == 0) ? 1.0f : 2.0f };
            store.Publish(CameraOverride{ .Eye = Vec3{ value, value, value }, .Forward = Vec3{ value, value, value }, .Up = Vec3{ value, value, value } });
        }
        isDone.store(true);
    } };

    int torn{ 0 };
    while (!isDone.load())
    {
        const std::optional<CameraOverride> pose{ store.Read(std::chrono::seconds{ 5 }) };
        if (!pose) continue;

        const float value{ pose->Eye.X };
        if (pose->Eye.Y != value || pose->Eye.Z != value || pose->Forward.X != value || pose->Forward.Y != value ||
            pose->Forward.Z != value || pose->Up.X != value || pose->Up.Y != value || pose->Up.Z != value) ++torn;
    }

    writer.join();
    EXPECT_EQ(torn, 0);
}