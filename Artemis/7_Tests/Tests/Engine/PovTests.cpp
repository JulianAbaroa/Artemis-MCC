#include <gtest/gtest.h>

import Common.Engine.Type;
import Engine.Base.Pov.Type;
import Engine.Base.Pov.State;
import Engine.Base.Pov.System;
import std;

// The POV of the view of user 0 (VW.9a, D340): the decision of the call, the request store (fresh, old, cleared, double buffer) and the status store. No game needed.

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Engine::Base::Pov::State::PovRequestStore;
    using Engine::Base::Pov::State::PovStatusStore;
    using Engine::Base::Pov::System::DecidePovCall;
    using Engine::Base::Pov::System::IsPovOnOtherPlayer;
    using Engine::Base::Pov::System::NextAssigned;
    using Engine::Base::Pov::Type::PovRequest;
    using Engine::Base::Pov::Type::PovStatus;
    using Engine::Base::Pov::Type::k_PovRequestMaxAge;

    constexpr std::uint32_t k_Bot{ 0x00030001 };
    constexpr std::uint32_t k_Other{ 0x00040002 };
    constexpr std::uint32_t k_Local{ 0x00010000 };
}

TEST(PovTests, AValidRequestDifferentFromTheViewIsAssigned)
{
    // The ownership of the view does not matter for a valid request.
    for (const bool owns : { false, true })
    {
        EXPECT_EQ(DecidePovCall(k_Bot, k_Local, k_Local, true, owns), std::optional<std::uint32_t>{ k_Bot });
        EXPECT_EQ(DecidePovCall(k_Bot, k_Other, k_Local, true, owns), std::optional<std::uint32_t>{ k_Bot });
        EXPECT_EQ(DecidePovCall(k_Bot, k_NoHandle, k_Local, true, owns), std::optional<std::uint32_t>{ k_Bot });

        // It does not need a local player.
        EXPECT_EQ(DecidePovCall(k_Bot, k_Local, k_NoHandle, true, owns), std::optional<std::uint32_t>{ k_Bot });
    }
}

TEST(PovTests, AValidRequestEqualToTheViewAssignsNothing)
{
    EXPECT_EQ(DecidePovCall(k_Bot, k_Bot, k_Local, true, true), std::nullopt);
    EXPECT_EQ(DecidePovCall(k_Local, k_Local, k_Local, true, false), std::nullopt);
}

TEST(PovTests, AnInvalidRequestGoesBackToTheLocalPlayerOnlyWhenTheServiceOwnsTheView)
{
    EXPECT_EQ(DecidePovCall(k_Bot, k_Bot, k_Local, false, true), std::optional<std::uint32_t>{ k_Local });
    EXPECT_EQ(DecidePovCall(k_Bot, k_Local, k_Local, false, true), std::nullopt);

    // Without the view it is not touched, even when it differs from the local player.
    EXPECT_EQ(DecidePovCall(k_Bot, k_Bot, k_Local, false, false), std::nullopt);

    // Without a local player nothing is touched.
    EXPECT_EQ(DecidePovCall(k_Bot, k_Bot, k_NoHandle, false, true), std::nullopt);
}

TEST(PovTests, NoRequestGoesBackToTheLocalPlayerOnlyWhenTheViewDiffersAndIsOwned)
{
    EXPECT_EQ(DecidePovCall(std::nullopt, k_Bot, k_Local, false, true), std::optional<std::uint32_t>{ k_Local });
    EXPECT_EQ(DecidePovCall(std::nullopt, k_NoHandle, k_Local, false, true), std::optional<std::uint32_t>{ k_Local });
    EXPECT_EQ(DecidePovCall(std::nullopt, k_Local, k_Local, false, true), std::nullopt);

    // The theater or a join in progress changed the view and this service never held it: it stays.
    EXPECT_EQ(DecidePovCall(std::nullopt, k_Bot, k_Local, false, false), std::nullopt);
    EXPECT_EQ(DecidePovCall(std::nullopt, k_NoHandle, k_Local, false, false), std::nullopt);
}

TEST(PovTests, NoRequestAndNoLocalPlayerTouchesNothing)
{
    EXPECT_EQ(DecidePovCall(std::nullopt, k_Bot, k_NoHandle, false, true), std::nullopt);
    EXPECT_EQ(DecidePovCall(std::nullopt, k_NoHandle, k_NoHandle, false, true), std::nullopt);
}

TEST(PovTests, AnAssignedPlayerThatIsNotLocalBecomesOwned)
{
    // The normal flow: the view goes to the bot, and keeps going while the engine shows it.
    EXPECT_EQ(NextAssigned(k_NoHandle, k_Bot, k_Local, k_Local), k_Bot);
    EXPECT_EQ(NextAssigned(k_Bot, std::nullopt, k_Bot, k_Local), k_Bot);

    // A change of bot.
    EXPECT_EQ(NextAssigned(k_Bot, k_Other, k_Bot, k_Local), k_Other);

    // Nothing owned and nobody called: the theater or a join in progress changed the view and it stays theirs.
    EXPECT_EQ(NextAssigned(k_NoHandle, std::nullopt, k_Bot, k_Local), k_NoHandle);
    EXPECT_EQ(NextAssigned(k_NoHandle, std::nullopt, k_NoHandle, k_Local), k_NoHandle);

    // A call that faulted gives nothing new.
    EXPECT_EQ(NextAssigned(k_Bot, std::nullopt, k_Bot, k_Local), k_Bot);
}

TEST(PovTests, TheOwnershipEndsWhenTheViewIsTheLocalPlayer)
{
    // The return call of this update.
    EXPECT_EQ(NextAssigned(k_Bot, k_Local, k_Bot, k_Local), k_NoHandle);

    // The request is the local player and the call set it (M1 b).
    EXPECT_EQ(NextAssigned(k_Bot, k_Local, k_Bot, k_Local), k_NoHandle);

    // The request ended or became invalid with the engine already on local, so there was no call (M1 a).
    EXPECT_EQ(NextAssigned(k_Bot, std::nullopt, k_Local, k_Local), k_NoHandle);

    // Without a local player nothing ends the ownership.
    EXPECT_EQ(NextAssigned(k_Bot, std::nullopt, k_NoHandle, k_NoHandle), k_Bot);
    EXPECT_EQ(NextAssigned(k_NoHandle, k_Bot, k_NoHandle, k_NoHandle), k_Bot);
}

TEST(PovTests, TheViewIsOnOtherPlayerWhenTheEngineShowsTheRequestedPlayerThatIsNotTheLocalOne)
{
    PovStatus status{};
    EXPECT_FALSE(IsPovOnOtherPlayer(status));

    status.RequestedHandle = k_Bot;
    status.LocalHandle = k_Local;
    status.EngineHandle = k_Local;
    EXPECT_FALSE(IsPovOnOtherPlayer(status));

    status.EngineHandle = k_Bot;
    EXPECT_TRUE(IsPovOnOtherPlayer(status));

    // The engine shows another player than the requested one (the change has not happened yet).
    status.EngineHandle = k_Other;
    EXPECT_FALSE(IsPovOnOtherPlayer(status));

    // A request for the local player is not another player.
    status.RequestedHandle = k_Local;
    status.EngineHandle = k_Local;
    EXPECT_FALSE(IsPovOnOtherPlayer(status));
}

TEST(PovTests, TheRequestStoreGivesWhatWasPublishedUntilItIsCleared)
{
    PovRequestStore store{};
    EXPECT_FALSE(store.Read(std::chrono::seconds{ 1 }).has_value());

    store.Publish(k_Bot);

    const std::optional<PovRequest> request{ store.Read(std::chrono::seconds{ 1 }) };
    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->PlayerHandle, k_Bot);

    store.Publish(k_Other);
    EXPECT_EQ(store.Read(std::chrono::seconds{ 1 })->PlayerHandle, k_Other);

    store.Clear();
    EXPECT_FALSE(store.Read(std::chrono::seconds{ 1 }).has_value());
}

TEST(PovTests, AnOldRequestIsTakenAsNone)
{
    PovRequestStore store{};
    store.Publish(k_Bot);

    std::this_thread::sleep_for(std::chrono::milliseconds{ 20 });

    EXPECT_FALSE(store.Read(std::chrono::milliseconds{ 5 }).has_value());
    EXPECT_TRUE(store.Read(std::chrono::seconds{ 5 }).has_value());

    // A new request is fresh again.
    store.Publish(k_Bot);
    EXPECT_TRUE(store.Read(std::chrono::milliseconds{ 5 }).has_value());
}

TEST(PovTests, TheMaxAgeOfARequestIsOneHundredMilliseconds)
{
    EXPECT_EQ(k_PovRequestMaxAge, std::chrono::milliseconds{ 100 });
}

TEST(PovTests, TheRequestStoreNeverGivesATornRequest)
{
    PovRequestStore store{};
    std::atomic<bool> isDone{ false };

    // The writer alternates two handles, and a reader only ever sees one of them.
    std::thread writer{ [&] {
        for (int i{ 0 }; i < 200000; ++i) store.Publish((i % 2 == 0) ? k_Bot : k_Other);
        isDone.store(true);
    } };

    int torn{ 0 };
    while (!isDone.load())
    {
        const std::optional<PovRequest> request{ store.Read(std::chrono::seconds{ 5 }) };
        if (request && request->PlayerHandle != k_Bot && request->PlayerHandle != k_Other) ++torn;
    }

    writer.join();
    EXPECT_EQ(torn, 0);
}

TEST(PovTests, TheStatusStoreKeepsTheHandlesTheCountsAndTheInstalledFlag)
{
    PovStatusStore store{};

    PovStatus status{ store.Read() };
    EXPECT_EQ(status.RequestedHandle, k_NoHandle);
    EXPECT_EQ(status.EngineHandle, k_NoHandle);
    EXPECT_EQ(status.LocalHandle, k_NoHandle);
    EXPECT_EQ(status.AssignedHandle, k_NoHandle);
    EXPECT_EQ(status.Calls, 0u);
    EXPECT_EQ(status.Reasserts, 0u);
    EXPECT_EQ(status.Faults, 0u);
    EXPECT_FALSE(status.IsInstalled);

    store.SetHandles(k_Bot, k_Local, k_Local, k_Bot);
    store.CountCall(false);
    store.CountCall(true);
    store.CountFault();
    store.SetInstalled(true);

    status = store.Read();
    EXPECT_EQ(status.RequestedHandle, k_Bot);
    EXPECT_EQ(status.EngineHandle, k_Local);
    EXPECT_EQ(status.LocalHandle, k_Local);
    EXPECT_EQ(status.AssignedHandle, k_Bot);
    EXPECT_EQ(status.Calls, 2u);
    EXPECT_EQ(status.Reasserts, 1u);
    EXPECT_EQ(status.Faults, 1u);
    EXPECT_TRUE(status.IsInstalled);

    // Reset starts a new engine clean, the faults and the installed flag included.
    PovStatusStore other{};
    other.SetHandles(k_Bot, k_Local, k_Local, k_Bot);
    other.CountCall(true);
    other.CountFault();
    other.SetInstalled(true);
    other.Reset();
    EXPECT_EQ(other.Read().AssignedHandle, k_NoHandle);
    EXPECT_EQ(other.Read().RequestedHandle, k_NoHandle);
    EXPECT_EQ(other.Read().Calls, 0u);
    EXPECT_EQ(other.Read().Faults, 0u);
    EXPECT_FALSE(other.Read().IsInstalled);

    // A new request zeroes the calls and the reasserts but not the faults.
    store.ResetCounts();
    status = store.Read();
    EXPECT_EQ(status.Calls, 0u);
    EXPECT_EQ(status.Reasserts, 0u);
    EXPECT_EQ(status.Faults, 1u);
}