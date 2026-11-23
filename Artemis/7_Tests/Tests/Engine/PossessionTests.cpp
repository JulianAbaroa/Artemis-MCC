#include <gtest/gtest.h>

import Common.Engine.Type;
import Control.Action.Type;
import Control.Client.System;
import Control.Client.Type;
import Engine.Control.Input.State;
import Engine.Control.Input.System;
import Engine.Control.Input.Type;
import Bridge.Codec.Tick.Type;
import std;

using namespace std::chrono_literals;

// The possession of a bot (VW.9c.2 B): the state the client keeps per possessed bot, which bot can be possessed, and what each player gets.

namespace
{
    using Common::Engine::Type::k_NoHandle;

    using Engine::Control::Input::State::PossessionStore;
    using Engine::Control::Input::State::PossessRequestStore;
    using Engine::Control::Input::System::FindPossessedBot;
    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::System::AddAgentFrame;
    using ::Control::Client::System::CloseAgentInput;
    using ::Control::Client::System::MakeAgentFrame;
    using ::Control::Client::Type::AgentInput;
    using ::Control::Client::Type::AgentInputState;
    using ::Control::Client::Type::ClientOutput;
    using ::Control::Client::Type::ClientSettings;
    using ::Control::Client::Type::k_AgentControlCount;
    using ::Control::Client::Type::ClientStep;
    using ::Control::Client::Type::UnitContext;
    using ::Control::Client::Type::k_EventReload;
    using ::Control::Client::Type::k_EventSwitchWeapon;
    using ::Control::Client::Type::k_EventZoomHeld;
    using ::Control::Client::Type::k_EventZoomPress;

    using Engine::Control::Input::System::AccumulateFrame;
    using Engine::Control::Input::System::PrepareFrame;
    using Engine::Control::Input::System::ShouldNeutralizeLocal;
    using Engine::Control::Input::System::TakeTickAction;
    using Engine::Control::Input::System::TickFrameAge;
    using Engine::Control::Input::Type::k_PossessMaxStaleTicks;
    using Engine::Control::Input::Type::ClientDriven;

    constexpr std::size_t k_Players{ 16 };

    auto NoBots() -> std::array<std::uint32_t, k_Players>
    {
        std::array<std::uint32_t, k_Players> handles{};
        handles.fill(k_NoHandle);
        return handles;
    }
}

TEST(PossessionStoreTests, StartsWithNothingPossessed)
{
    PossessionStore store{};

    const auto status{ store.GetStatus() };
    EXPECT_EQ(status.Player, k_NoHandle);
    EXPECT_EQ(status.Ticks, 0u);
    EXPECT_EQ(store.Find(), nullptr);
}

TEST(PossessionStoreTests, BeginStartsANewStateAndThenGoesOnWithIt)
{
    PossessionStore store{};

    EXPECT_TRUE(store.Begin(3, 0x1003));
    ASSERT_NE(store.Find(), nullptr);
    store.Find()->State.Yaw = 1.5f;
    store.Find()->Unit = 77;

    EXPECT_FALSE(store.Begin(3, 0x1003));
    EXPECT_FLOAT_EQ(store.Find()->State.Yaw, 1.5f);
    EXPECT_EQ(store.Find()->Unit, 77u);
    EXPECT_EQ(store.GetStatus().Player, 0x1003u);
}

TEST(PossessionStoreTests, AnotherSaltInTheSameSlotIsAnotherPossession)
{
    PossessionStore store{};
    store.Begin(3, 0x1003);
    store.Find()->State.Yaw = 1.5f;

    EXPECT_TRUE(store.Begin(3, 0x2003));
    EXPECT_FLOAT_EQ(store.Find()->State.Yaw, 0.0f);
}

TEST(PossessionStoreTests, OnlyOneBotIsPossessedAtATime)
{
    PossessionStore store{};
    store.Begin(3, 0x1003);

    EXPECT_TRUE(store.Begin(5, 0x1005));
    ASSERT_NE(store.Find(), nullptr);
    EXPECT_EQ(store.Find()->Player, 5u);
}

TEST(PossessionStoreTests, ReleaseDropsTheStateAndTheMark)
{
    PossessionStore store{};
    store.Begin(3, 0x1003);
    store.Find()->State.Yaw = 1.5f;

    store.Release();

    EXPECT_EQ(store.Find(), nullptr);
    EXPECT_EQ(store.GetStatus().Player, k_NoHandle);

    // note: A new possession of the same bot does not inherit the old aim.
    EXPECT_TRUE(store.Begin(3, 0x1003));
    EXPECT_FLOAT_EQ(store.Find()->State.Yaw, 0.0f);
}

TEST(PossessionStoreTests, IsActiveFollowsBeginAndRelease)
{
    PossessionStore store{};
    EXPECT_FALSE(store.IsActive());

    store.Begin(3, 0x1003);
    EXPECT_TRUE(store.IsActive());

    store.Release();
    EXPECT_FALSE(store.IsActive());
}

TEST(PossessionStoreTests, CountsDeliveredAndUnsupportedTicks)
{
    PossessionStore store{};
    store.Begin(3, 0x1003);

    store.CountTick(true, false);
    store.CountTick(true, true);
    store.CountTick(false, false);

    const auto status{ store.GetStatus() };
    EXPECT_EQ(status.Ticks, 3u);
    EXPECT_EQ(status.Delivered, 2u);
    EXPECT_EQ(status.Unsupported, 1u);
}

TEST(PossessionStoreTests, NewPossessionResetsTheCounters)
{
    PossessionStore store{};
    store.Begin(3, 0x1003);
    store.CountTick(true, true);

    store.Begin(4, 0x1004);

    const auto status{ store.GetStatus() };
    EXPECT_EQ(status.Ticks, 0u);
    EXPECT_EQ(status.Delivered, 0u);
    EXPECT_EQ(status.Unsupported, 0u);
}

TEST(PossessionStoreTests, UnitMissingIsReportedOnceForEachPossession)
{
    PossessionStore store{};
    store.Begin(3, 0x1003);

    EXPECT_TRUE(store.MarkUnitMissing());
    EXPECT_FALSE(store.MarkUnitMissing());
    EXPECT_TRUE(store.GetStatus().IsUnitMissing);

    store.Begin(4, 0x1004);
    EXPECT_FALSE(store.GetStatus().IsUnitMissing);
    EXPECT_TRUE(store.MarkUnitMissing());
}

TEST(PossessRequestStoreTests, StartsWithoutARequest)
{
    const PossessRequestStore store{};

    EXPECT_FALSE(store.Read(100ms).has_value());
}

TEST(PossessRequestStoreTests, PublishedRequestIsReadUntilItIsCleared)
{
    PossessRequestStore store{};
    const auto now{ std::chrono::steady_clock::now() };

    store.Publish(0x1003, now);
    EXPECT_EQ(store.Read(100ms, now), std::optional<std::uint32_t>{ 0x1003 });

    store.Clear();
    EXPECT_FALSE(store.Read(100ms, now).has_value());
}

TEST(PossessRequestStoreTests, ARequestOlderThanTheMaxAgeIsNoRequest)
{
    PossessRequestStore store{};
    const auto now{ std::chrono::steady_clock::now() };

    store.Publish(0x1003, now);

    EXPECT_TRUE(store.Read(100ms, now + 100ms).has_value());
    EXPECT_FALSE(store.Read(100ms, now + 101ms).has_value());
}

TEST(PossessRequestStoreTests, AStampAheadOfTheReaderIsFreshNotOld)
{
    PossessRequestStore store{};
    const auto now{ std::chrono::steady_clock::now() };

    store.Publish(0x1003, now + 2ms);

    EXPECT_TRUE(store.Read(100ms, now).has_value());
}

TEST(PossessRequestStoreTests, ANewPublishReplacesTheHandleAndRefreshesTheAge)
{
    PossessRequestStore store{};
    const auto now{ std::chrono::steady_clock::now() };

    store.Publish(0x1003, now);
    store.Publish(0x1005, now + 90ms);

    EXPECT_EQ(store.Read(100ms, now + 150ms), std::optional<std::uint32_t>{ 0x1005 });
}

TEST(PossessRequestStoreTests, TheAgeHoldsWhenTheClockOfTheWordWraps)
{
    PossessRequestStore store{};

    // note: The word keeps 31 bits of milliseconds, so a write just before the wrap is still young just after it.
    const std::chrono::steady_clock::time_point beforeWrap{ std::chrono::milliseconds{ 0x7FFFFFF0 } };
    store.Publish(0x1003, beforeWrap);

    EXPECT_TRUE(store.Read(100ms, beforeWrap + 50ms).has_value());
    EXPECT_FALSE(store.Read(100ms, beforeWrap + 200ms).has_value());
}

TEST(PossessionStoreTests, OnlyTheThreadThatBeganThePossessionIsTheTickThread)
{
    PossessionStore store{};
    store.Begin(3, 0x1003);
    EXPECT_TRUE(store.IsTickThread());
    EXPECT_EQ(store.GetTickThread(), std::this_thread::get_id());

    bool isTickThread{ true };
    std::thread other{ [&] { isTickThread = store.IsTickThread(); } };
    other.join();
    EXPECT_FALSE(isTickThread);
}

TEST(PossessionStoreTests, DisableReportsTheFirstTimeOnlyAndSticks)
{
    PossessionStore store{};
    EXPECT_FALSE(store.IsDisabled());

    EXPECT_TRUE(store.Disable());
    EXPECT_FALSE(store.Disable());
    EXPECT_TRUE(store.IsDisabled());

    store.Release();
    EXPECT_TRUE(store.IsDisabled());
}

TEST(PossessionStoreTests, TheThreadsAreLoggedOncePerPossession)
{
    PossessionStore store{};
    store.Begin(3, 0x1003);

    EXPECT_TRUE(store.MarkThreadsLogged());
    EXPECT_FALSE(store.MarkThreadsLogged());

    store.Begin(4, 0x1004);
    EXPECT_TRUE(store.MarkThreadsLogged());
}

TEST(FindPossessedBotTests, FindsTheSlotOfTheBotWithThatHandle)
{
    auto bots{ NoBots() };
    bots[2] = 0x4002;
    bots[7] = 0x1007;

    EXPECT_EQ(FindPossessedBot(0x1007u, bots), std::optional<std::size_t>{ 7 });
    EXPECT_EQ(FindPossessedBot(0x4002u, bots), std::optional<std::size_t>{ 2 });
}

TEST(FindPossessedBotTests, ANoRequestOrANoHandleGivesNothing)
{
    auto bots{ NoBots() };
    bots[2] = 0x4002;

    EXPECT_FALSE(FindPossessedBot(std::nullopt, bots).has_value());
    EXPECT_FALSE(FindPossessedBot(k_NoHandle, bots).has_value());
}

TEST(FindPossessedBotTests, AHumanIsNeverPossessed)
{
    auto bots{ NoBots() };
    bots[2] = 0x4002;

    // note: The human of slot 0 is not in the list of bots, so its handle finds nothing.
    EXPECT_FALSE(FindPossessedBot(0x1000u, bots).has_value());
}

TEST(FindPossessedBotTests, AnotherSaltInTheSlotOfABotGivesNothing)
{
    auto bots{ NoBots() };
    bots[2] = 0x4002;

    EXPECT_FALSE(FindPossessedBot(0x5002u, bots).has_value());
}

TEST(ShouldNeutralizeLocalTests, TheHumanLocalGoesNeutralWhileABotIsPossessed)
{
    EXPECT_TRUE(ShouldNeutralizeLocal(3, 0, 0, 0x1u));
}

TEST(ShouldNeutralizeLocalTests, NothingPossessedChangesNothing)
{
    EXPECT_FALSE(ShouldNeutralizeLocal(std::nullopt, 0, 0, 0x1u));
}

TEST(ShouldNeutralizeLocalTests, ALocalThatIsAnAgentIsNotTouched)
{
    // note: Player 0 has an override of Python: it is an agent, the input of the user is not for it.
    EXPECT_FALSE(ShouldNeutralizeLocal(3, 0, 0x1u, 0x1u));
}

TEST(ShouldNeutralizeLocalTests, ALocalWithoutAnEngineActionIsNotTouched)
{
    EXPECT_FALSE(ShouldNeutralizeLocal(3, 0, 0, 0));
}

TEST(ShouldNeutralizeLocalTests, WithoutALocalNobodyIsNeutral)
{
    EXPECT_FALSE(ShouldNeutralizeLocal(3, std::nullopt, 0, 0x1u));
}

TEST(ShouldNeutralizeLocalTests, ALocalThatIsThePossessedPlayerIsNotNeutral)
{
    EXPECT_FALSE(ShouldNeutralizeLocal(0, 0, 0, 0x1u));
}

TEST(ShouldNeutralizeLocalTests, TheOverrideOfPythonOnTheBotDoesNotChangeTheLocal)
{
    EXPECT_TRUE(ShouldNeutralizeLocal(3, 0, 1u << 3, 0x1u));
}

TEST(ShouldNeutralizeLocalTests, ASlotOutOfRangeGivesNothing)
{
    EXPECT_FALSE(ShouldNeutralizeLocal(k_Players, 0, 0, 0x1u));
    EXPECT_FALSE(ShouldNeutralizeLocal(3, k_Players, 0, 0x1u));
}

namespace
{
    constexpr std::uint32_t k_Unit{ 7 };

    auto MakeUnit(std::uint32_t unit, float yaw, float pitch) -> UnitClientState
    {
        UnitClientState state{};
        state.Unit = unit;
        state.Facing = { yaw, pitch };
        return state;
    }

    // Runs one frame of the client: what the 0x5df80 hook does, with the output the test gives.
    auto RunFrame(ClientDriven& bot, const ClientOutput& output) -> void
    {
        const UnitClientState unit{ MakeUnit(k_Unit, 1.0f, 0.0f) };
        ClientStep step{};
        step.State = PrepareFrame(bot, unit);
        step.Output = output;
        AccumulateFrame(bot, step, UnitContext{});
    }
}

TEST(PossessedFrameTests, SeveralTicksInOneFrameDeliverAPressOnce)
{
    ClientDriven bot{};
    ClientOutput output{};
    output.Events = k_EventReload | k_EventSwitchWeapon | k_EventZoomPress;
    RunFrame(bot, output);

    const auto first{ TakeTickAction(bot) };
    EXPECT_EQ(first.Override.ClientEvents, 1);
    EXPECT_TRUE(first.Requests.IsWeaponSwitch);
    EXPECT_TRUE(first.Requests.IsZoom);

    const auto second{ TakeTickAction(bot) };
    EXPECT_EQ(second.Override.ClientEvents, 0);
    EXPECT_FALSE(second.Requests.IsWeaponSwitch);
    EXPECT_FALSE(second.Requests.IsZoom);
}

TEST(PossessedFrameTests, FramesWithoutATickAccumulateTheAimAndKeepThePress)
{
    ClientDriven oneStep{};
    ClientOutput big{};
    big.YawDelta = 0.3f;
    big.PitchDelta = 0.06f;
    RunFrame(oneStep, big);

    ClientDriven frames{};
    ClientOutput press{};
    press.YawDelta = 0.1f;
    press.PitchDelta = 0.02f;
    press.Events = k_EventZoomPress;
    RunFrame(frames, press);

    ClientOutput look{};
    look.YawDelta = 0.1f;
    look.PitchDelta = 0.02f;
    RunFrame(frames, look);
    RunFrame(frames, look);

    EXPECT_NEAR(frames.State.Yaw, oneStep.State.Yaw, 1e-5f);
    EXPECT_NEAR(frames.State.Pitch, oneStep.State.Pitch, 1e-5f);

    const auto action{ TakeTickAction(frames) };
    EXPECT_NEAR(action.Override.Yaw, oneStep.State.Yaw, 1e-5f);
    EXPECT_TRUE(action.Requests.IsZoom);
    EXPECT_FALSE(TakeTickAction(frames).Requests.IsZoom);
}

TEST(PossessedFrameTests, TheHeldStateStaysAndTheButtonsOfBetweenTicksAreNotLost)
{
    ClientDriven bot{};
    ClientOutput tap{};
    tap.Buttons = 1ull << 1;
    tap.Events = k_EventZoomHeld;
    tap.MoveForward = 1.0f;
    RunFrame(bot, tap);

    ClientOutput idle{};
    idle.Events = k_EventZoomHeld;
    idle.MoveForward = 1.0f;
    RunFrame(bot, idle);

    const auto action{ TakeTickAction(bot) };
    EXPECT_NE(action.Override.Buttons & (1ull << 1), 0u);
    EXPECT_EQ(action.Override.ThrottleForward, 1.0f);

    // note: With no new frame the held move stays, the tap does not repeat from the pending buffer.
    EXPECT_EQ(TakeTickAction(bot).Override.ThrottleForward, 1.0f);
}

TEST(PossessedFrameTests, ABotWithoutAFrameYetIsStale)
{
    ClientDriven bot{};

    EXPECT_TRUE(TickFrameAge(bot));
}

TEST(PossessedFrameTests, AFrameThatStopsRunningGoesNeutralAndForgetsTheHeldState)
{
    ClientDriven bot{};
    ClientOutput held{};
    held.Buttons = 1ull << 1;
    held.MoveForward = 1.0f;
    held.FireTrigger = 1.0f;
    RunFrame(bot, held);

    // note: Ticks with no new frame re-deliver the held state, up to the limit.
    for (std::uint32_t tick{ 0 }; tick < k_PossessMaxStaleTicks; ++tick)
    {
        ASSERT_FALSE(TickFrameAge(bot));
        EXPECT_EQ(TakeTickAction(bot).Override.ThrottleForward, 1.0f);
    }

    EXPECT_TRUE(TickFrameAge(bot));
    EXPECT_EQ(bot.Last.MoveForward, 0.0f);
    EXPECT_EQ(bot.Last.Buttons, 0u);
    EXPECT_EQ(bot.Last.FireTrigger, 0.0f);
}

TEST(PossessedFrameTests, ANewFrameMakesTheBotFreshAgain)
{
    ClientDriven bot{};
    RunFrame(bot, ClientOutput{});
    for (std::uint32_t tick{ 0 }; tick < k_PossessMaxStaleTicks; ++tick) ASSERT_FALSE(TickFrameAge(bot));
    ASSERT_TRUE(TickFrameAge(bot));

    RunFrame(bot, ClientOutput{});

    EXPECT_FALSE(TickFrameAge(bot));
}

TEST(PossessedFrameTests, TheLargestTriggerOfTheFramesIsDelivered)
{
    ClientDriven bot{};
    ClientOutput down{};
    down.FireTrigger = 1.0f;
    RunFrame(bot, down);
    RunFrame(bot, ClientOutput{});

    EXPECT_EQ(TakeTickAction(bot).Override.FireTrigger, 1.0f);
    EXPECT_EQ(TakeTickAction(bot).Override.FireTrigger, 0.0f);
}

TEST(PossessedFrameTests, ANewUnitStartsTheAimFromTheUnitAndForgetsTheOldRequests)
{
    ClientDriven bot{};
    ClientOutput output{};
    output.Events = k_EventReload;
    output.YawDelta = 0.5f;
    RunFrame(bot, output);
    ASSERT_TRUE(bot.HasFrame);

    const UnitClientState other{ MakeUnit(k_Unit + 1, 2.0f, 0.25f) };
    const auto state{ PrepareFrame(bot, other) };

    EXPECT_FALSE(bot.HasFrame);
    EXPECT_EQ(bot.Pending.Events, 0u);
    EXPECT_FLOAT_EQ(state.Yaw, 2.0f);
    EXPECT_FLOAT_EQ(state.Pitch, 0.25f);
}

TEST(TickPossessionTests, ATickMarksNoPlayerByDefault)
{
    const Bridge::Codec::Tick::Type::Tick tick{};

    EXPECT_EQ(tick.PossessedPlayer, k_NoHandle);
}

namespace
{
    constexpr std::int32_t k_WindowTicks{ 4 };

    // Adds the synthetic frames of one window of an AgentInput to the reducer of the store, the way the frames of the possessed client are added.
    auto AddWindow(PossessionStore& store, const AgentInput& input, AgentInputState& state) -> void
    {
        for (std::int32_t tick{ 0 }; tick < k_WindowTicks; ++tick)
        {
            const auto frame{ MakeAgentFrame(input, tick, k_WindowTicks, ClientSettings{}, 0.0f, state) };
            AddAgentFrame(store.GetReducer(), frame, input.YawDelta / k_WindowTicks, input.PitchDelta / k_WindowTicks);
        }
    }

    // Closes the window the way the bridge does, through the store: the reducer gives the window and the store keeps it.
    auto CloseWindow(PossessionStore& store) -> AgentInput
    {
        store.SetWindow(CloseAgentInput(store.GetReducer()));
        return store.GetWindow();
    }
}

// The imitation of the possessed bot (VW.9c.3 D, D364): the frames of each window are reduced to one AgentInput, kept as the last window.
TEST(PossessionWindowTests, EachControlInEachOfTheFourStatesComesOutAsTheStatesOfTheAgentInput)
{
    for (std::uint32_t control{ 0 }; control < k_AgentControlCount; ++control)
    {
        const std::uint32_t bit{ 1u << control };

        // nothing, tap, press and hold, and keep holding (it was down at the start of the window).
        struct Case { std::uint32_t Level; std::uint32_t Edge; bool WasDown; };
        for (const Case& item : { Case{ 0, 0, false }, Case{ 0, bit, false }, Case{ bit, bit, false }, Case{ bit, 0, true } })
        {
            PossessionStore store{};
            store.Begin(0, 0x10000);
            AgentInputState state{};
            if (item.WasDown)
            {
                AddWindow(store, AgentInput{ .Level = bit, .Edge = bit }, state);
                CloseWindow(store);
            }

            AddWindow(store, AgentInput{ .Level = item.Level, .Edge = item.Edge }, state);
            const AgentInput window{ CloseWindow(store) };
            EXPECT_EQ(window.Level, item.Level) << "control " << control;
            EXPECT_EQ(window.Edge, item.Edge) << "control " << control;
        }
    }
}

TEST(PossessionWindowTests, TheAxesAndTheLookAskedAreTheOnesOfTheWindow)
{
    PossessionStore store{};
    store.Begin(0, 0x10000);
    AgentInputState state{};

    AddWindow(store, AgentInput{ .MoveForward = 1.0f, .MoveLeft = -0.5f, .YawDelta = 0.4f, .PitchDelta = -0.2f }, state);
    const AgentInput window{ CloseWindow(store) };

    EXPECT_FLOAT_EQ(window.MoveForward, 1.0f);
    EXPECT_FLOAT_EQ(window.MoveLeft, -0.5f);
    EXPECT_NEAR(window.YawDelta, 0.4f, 1e-5f);
    EXPECT_NEAR(window.PitchDelta, -0.2f, 1e-5f);

    // The next window starts clean: its look does not add to the one before.
    AddWindow(store, AgentInput{ .YawDelta = 0.1f }, state);
    EXPECT_NEAR(CloseWindow(store).YawDelta, 0.1f, 1e-5f);
}

TEST(PossessionWindowTests, TheWindowIsZeroWithoutPossessionAndStartsFreshWithEach)
{
    PossessionStore store{};
    EXPECT_EQ(store.GetWindow().Level, 0u);

    store.Begin(0, 0x10000);
    AgentInputState state{};
    AddWindow(store, AgentInput{ .MoveForward = 1.0f, .Level = 1, .Edge = 1 }, state);
    EXPECT_EQ(CloseWindow(store).Level, 1u);

    // Going on with the same bot keeps the window. Another bot, or the end of the possession, forget it.
    store.Begin(0, 0x10000);
    EXPECT_EQ(store.GetWindow().Level, 1u);

    store.Begin(1, 0x20000);
    EXPECT_EQ(store.GetWindow().Level, 0u);
    EXPECT_FLOAT_EQ(store.GetReducer().MoveForward, 0.0f);

    AddWindow(store, AgentInput{ .Level = 2, .Edge = 2 }, state);
    CloseWindow(store);
    store.Release();
    EXPECT_EQ(store.GetWindow().Level, 0u);
    EXPECT_EQ(store.GetReducer().Level, 0u);
}
