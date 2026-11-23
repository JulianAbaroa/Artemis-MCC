#include <gtest/gtest.h>

import Control.Action.System;
import Control.Action.Type;
import Control.Client.System;
import Control.Client.Type;
import Engine.Control.Input.State;
import Engine.Control.Input.System;
import Engine.Control.Input.Type;
import std;

// The client of Artemis for the agents (VW.9c.3 C): the AgentInput the store holds, the window the tick is in, the frame each tick plays through the same
// StepClient as the possession, and the settings Artemis fixes for an agent.

namespace
{
    using ::Control::Action::Type::ActionOverride;
    using ::Control::Action::Type::PossessedAction;
    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::System::StepClient;
    using ::Control::Client::Type::AgentControl;
    using ::Control::Client::Type::AgentInput;
    using ::Control::Client::Type::ClientSettings;
    using ::Control::Client::Type::ClientState;
    using ::Control::Client::Type::ClientStep;
    using ::Control::Client::Type::InputEvent;
    using ::Control::Client::Type::InputFrame;
    using ::Control::Client::Type::UnitContext;

    using Engine::Control::Input::State::AgentClientStore;
    using Engine::Control::Input::State::PlayerActionStore;
    using Engine::Control::Input::System::AccumulateFrame;
    using Engine::Control::Input::System::AdvanceAgentWindow;
    using Engine::Control::Input::System::GetAgentTickInput;
    using Engine::Control::Input::System::MakeAgentTickFrame;
    using Engine::Control::Input::System::PrepareAgentFrame;
    using Engine::Control::Input::System::ReadAgentClientSettings;
    using Engine::Control::Input::System::ResetAgentUnit;
    using Engine::Control::Input::System::ShouldRunAgent;
    using Engine::Control::Input::System::TakeTickAction;
    using Engine::Control::Input::Type::AgentClient;
    using Engine::Control::Input::Type::AgentDelivery;
    using Engine::Control::Input::Type::k_ClockSlot;
    using Engine::Control::Input::Type::k_ClockTicksPerSecondOffset;
    using Engine::Control::Input::Type::k_ClockTickSecondsOffset;
    using Engine::Control::Input::Type::k_ControlTagJumpTimerOffset;
    using Engine::Control::Input::Type::k_ControlTagZoomCancelOffset;
    using Engine::Control::Input::Type::k_HoldThresholdOffset;

    constexpr float k_Dt{ 1.0f / 60.0f };

    auto Bit(AgentControl control) -> std::uint32_t { return 1u << static_cast<std::uint32_t>(control); }

    auto MakeUnit(std::uint32_t unit) -> UnitClientState
    {
        UnitClientState state{};
        state.Unit = unit;
        state.Facing = { 1.0f, 0.0f };
        return state;
    }

    auto MakeDelivery(const AgentInput& input, std::uint32_t windowTicks, std::uint64_t sequence, float turnLimit = 0.0f) -> AgentDelivery
    {
        return AgentDelivery{ .Input = input, .WindowTicks = windowTicks, .TurnLimit = turnLimit, .Sequence = sequence };
    }

    // One tick of the client of an agent: what AgentClientService does, with a context that knows every fact and has a unit on foot.
    auto RunTick(AgentClient& agent, const AgentDelivery& delivery, const UnitClientState& unit) -> PossessedAction
    {
        const ClientSettings settings{};
        const std::uint32_t tick{ AdvanceAgentWindow(agent, delivery.Sequence) };
        const ClientState state{ PrepareAgentFrame(agent, unit) };
        const InputFrame frame{ MakeAgentTickFrame(agent, delivery, tick, settings) };

        ClientStep step{ StepClient(frame, state, UnitContext{ .HasUnit = true }, settings, k_Dt) };
        AccumulateFrame(agent.Client, step, UnitContext{ .HasUnit = true });
        return TakeTickAction(agent.Client);
    }

    template<typename T>
    auto Put(std::vector<std::uint8_t>& bytes, std::size_t offset, T value) -> void
    {
        std::memcpy(bytes.data() + offset, &value, sizeof(T));
    }
}

TEST(AgentInputStoreTests, EveryDeliveryRaisesTheSequence)
{
    PlayerActionStore store{};
    store.SetAgentInput(3, AgentInput{ .MoveForward = 1.0f }, 4, 0.0f);
    store.SetAgentInput(3, AgentInput{ .MoveForward = -1.0f }, 6, 0.25f);
    store.SetAgentInput(5, AgentInput{}, 4, 0.0f);

    std::array<std::optional<AgentDelivery>, 16> deliveries{};
    store.CopyAgentInputs(deliveries);

    ASSERT_TRUE(deliveries[3].has_value());
    EXPECT_EQ(deliveries[3]->Sequence, 2u);
    EXPECT_EQ(deliveries[3]->Input.MoveForward, -1.0f);
    EXPECT_EQ(deliveries[3]->WindowTicks, 6u);
    EXPECT_EQ(deliveries[3]->TurnLimit, 0.25f);
    ASSERT_TRUE(deliveries[5].has_value());
    EXPECT_EQ(deliveries[5]->Sequence, 3u);
    EXPECT_FALSE(deliveries[0].has_value());
}

TEST(AgentInputStoreTests, AnOverrideAndAnAgentInputExcludeEachOtherAndTheLastOneWins)
{
    PlayerActionStore store{};
    std::array<std::optional<AgentDelivery>, 16> deliveries{};
    std::array<std::optional<ActionOverride>, 16> overrides{};

    store.SetAgentInput(2, AgentInput{}, 4, 0.0f);
    store.SetOverride(2, ActionOverride{ .ThrottleForward = 1.0f });
    store.CopyAgentInputs(deliveries);
    store.CopyOverrides(overrides);
    EXPECT_FALSE(deliveries[2].has_value());
    EXPECT_TRUE(overrides[2].has_value());

    store.SetAgentInput(2, AgentInput{}, 4, 0.0f);
    store.CopyActions(overrides, deliveries);
    EXPECT_TRUE(deliveries[2].has_value());
    EXPECT_FALSE(overrides[2].has_value());
}

TEST(AgentInputStoreTests, ATurnLimitThatIsNotFiniteOrIsNegativeIsKeptAsNoLimit)
{
    PlayerActionStore store{};
    store.SetAgentInput(0, AgentInput{}, 4, std::numeric_limits<float>::quiet_NaN());
    store.SetAgentInput(1, AgentInput{}, 4, -0.5f);
    store.SetAgentInput(2, AgentInput{}, 4, std::numeric_limits<float>::infinity());

    std::array<std::optional<AgentDelivery>, 16> deliveries{};
    store.CopyAgentInputs(deliveries);

    for (std::size_t player{ 0 }; player < 3; ++player)
    {
        ASSERT_TRUE(deliveries[player].has_value());
        EXPECT_EQ(deliveries[player]->TurnLimit, 0.0f);
    }
}

TEST(AgentInputStoreTests, ReleasingTheOverrideLeavesTheAgentInput)
{
    PlayerActionStore store{};
    store.SetAgentInput(2, AgentInput{}, 4, 0.0f);
    store.SetOverride(2, std::nullopt);

    std::array<std::optional<AgentDelivery>, 16> deliveries{};
    store.CopyAgentInputs(deliveries);
    EXPECT_TRUE(deliveries[2].has_value());
}

TEST(AgentInputStoreTests, ClearStopsOnlyThatPlayerAndTheWindowHasAtLeastOneTick)
{
    PlayerActionStore store{};
    store.SetAgentInput(1, AgentInput{}, 0, 0.0f);
    store.SetAgentInput(2, AgentInput{}, 4, 0.0f);
    store.ClearAgentInput(1);
    store.SetAgentInput(16, AgentInput{}, 4, 0.0f);
    store.ClearAgentInput(99);

    std::array<std::optional<AgentDelivery>, 16> deliveries{};
    store.CopyAgentInputs(deliveries);
    EXPECT_FALSE(deliveries[1].has_value());
    EXPECT_TRUE(deliveries[2].has_value());

    store.SetAgentInput(1, AgentInput{}, 0, 0.0f);
    store.CopyAgentInputs(deliveries);
    ASSERT_TRUE(deliveries[1].has_value());
    EXPECT_EQ(deliveries[1]->WindowTicks, 1u);
}

TEST(AgentInputStoreTests, CleanupDropsTheAgentInputs)
{
    PlayerActionStore store{};
    store.SetAgentInput(4, AgentInput{}, 4, 0.0f);
    store.Cleanup();

    std::array<std::optional<AgentDelivery>, 16> deliveries{};
    store.CopyAgentInputs(deliveries);
    EXPECT_FALSE(deliveries[4].has_value());
}

TEST(AgentClientStoreTests, BeginStartsOnceAndGoesOnWhileActive)
{
    AgentClientStore store{};
    EXPECT_EQ(store.Find(2), nullptr);

    EXPECT_TRUE(store.Begin(2));
    ASSERT_NE(store.Find(2), nullptr);
    store.Find(2)->Sequence = 9;
    EXPECT_FALSE(store.Begin(2));
    EXPECT_EQ(store.Find(2)->Sequence, 9u);
    EXPECT_EQ(store.GetStatus().ActiveMask, 1u << 2);
}

TEST(AgentClientStoreTests, CountsTheTicksOfEachPlayerAndKeepsThemAfterTheEnd)
{
    AgentClientStore store{};
    store.Begin(1);
    store.Begin(3);
    store.CountTick(1, true, false, false);
    store.CountTick(1, true, true, true);
    store.CountTick(1, false, false, false);
    store.CountTick(3, true, false, true);

    auto status{ store.GetStatus() };
    EXPECT_EQ(status.Players[1].Ticks, 3u);
    EXPECT_EQ(status.Players[1].Delivered, 2u);
    EXPECT_EQ(status.Players[1].Unsupported, 1u);
    EXPECT_EQ(status.Players[1].UnknownRead, 1u);
    EXPECT_EQ(status.Players[3].UnknownRead, 1u);

    store.End(1);
    status = store.GetStatus();
    EXPECT_EQ(status.ActiveMask, 1u << 3);
    EXPECT_EQ(status.Players[1].Ticks, 3u);
    EXPECT_EQ(store.Find(1), nullptr);
}

TEST(AgentClientStoreTests, ANewBeginAfterTheEndZeroesTheCountersAndTheState)
{
    AgentClientStore store{};
    store.Begin(1);
    store.Find(1)->TickInWindow = 7;
    store.CountTick(1, true, true, true);
    store.End(1);

    EXPECT_TRUE(store.Begin(1));
    EXPECT_EQ(store.GetStatus().Players[1].Ticks, 0u);
    EXPECT_EQ(store.Find(1)->TickInWindow, 0u);
    EXPECT_FALSE(store.Find(1)->HasSequence);
}

TEST(AgentClientStoreTests, OutOfRangePlayersStartNothing)
{
    AgentClientStore store{};
    EXPECT_FALSE(store.Begin(16));
    EXPECT_EQ(store.Find(16), nullptr);
    store.CountTick(16, true, true, true);
    store.End(16);
    EXPECT_EQ(store.GetStatus().ActiveMask, 0u);
}

TEST(AgentWindowTests, ANewSequenceStartsAtTickZeroAndTheSameOneCounts)
{
    AgentClient agent{};
    EXPECT_EQ(AdvanceAgentWindow(agent, 5), 0u);
    EXPECT_EQ(AdvanceAgentWindow(agent, 5), 1u);
    EXPECT_EQ(AdvanceAgentWindow(agent, 5), 2u);
    EXPECT_EQ(AdvanceAgentWindow(agent, 6), 0u);
    EXPECT_EQ(AdvanceAgentWindow(agent, 6), 1u);
}

TEST(AgentWindowTests, ASequenceOfZeroIsAnOrdinaryFirstDelivery)
{
    AgentClient agent{};
    EXPECT_EQ(AdvanceAgentWindow(agent, 0), 0u);
    EXPECT_EQ(AdvanceAgentWindow(agent, 0), 1u);
}

TEST(AgentWindowTests, TheTickSaturatesInsteadOfWrapping)
{
    AgentClient agent{};
    AdvanceAgentWindow(agent, 1);
    agent.TickInWindow = (std::numeric_limits<std::uint32_t>::max)();
    EXPECT_EQ(AdvanceAgentWindow(agent, 1), (std::numeric_limits<std::uint32_t>::max)());
}

TEST(AgentWindowTests, PastTheWindowTheInputKeepsWhatIsHeldAndTheAxesButNoPressAndNoLook)
{
    const AgentDelivery delivery{ MakeDelivery(AgentInput{ .MoveForward = 1.0f, .MoveLeft = -0.5f, .YawDelta = 0.4f, .PitchDelta = 0.2f,
        .Level = Bit(AgentControl::Fire) | Bit(AgentControl::Jump), .Edge = Bit(AgentControl::Fire) | Bit(AgentControl::Reload) }, 4, 1) };

    const AgentInput inside{ GetAgentTickInput(delivery, 3) };
    EXPECT_EQ(inside.Edge, delivery.Input.Edge);
    EXPECT_EQ(inside.YawDelta, 0.4f);

    const AgentInput past{ GetAgentTickInput(delivery, 4) };
    EXPECT_EQ(past.Level, delivery.Input.Level);
    EXPECT_EQ(past.Edge, 0u);
    EXPECT_EQ(past.MoveForward, 1.0f);
    EXPECT_EQ(past.MoveLeft, -0.5f);
    EXPECT_EQ(past.YawDelta, 0.0f);
    EXPECT_EQ(past.PitchDelta, 0.0f);
}

TEST(AgentWindowTests, PossessionWinsOverTheAgentInput)
{
    EXPECT_TRUE(ShouldRunAgent(true, 3, std::nullopt));
    EXPECT_TRUE(ShouldRunAgent(true, 3, std::optional<std::size_t>{ 4 }));
    EXPECT_FALSE(ShouldRunAgent(true, 3, std::optional<std::size_t>{ 3 }));
    EXPECT_FALSE(ShouldRunAgent(false, 3, std::nullopt));
}

TEST(AgentTickTests, AHeldControlIsDownOnEveryTickAndItsDownCountKeepsCountingPastTheWindow)
{
    AgentClient agent{};
    const AgentDelivery delivery{ MakeDelivery(AgentInput{ .Level = Bit(AgentControl::Fire), .Edge = Bit(AgentControl::Fire) }, 4, 1) };

    for (std::uint32_t tick{ 0 }; tick < 10; ++tick)
    {
        const InputFrame frame{ MakeAgentTickFrame(agent, delivery, AdvanceAgentWindow(agent, 1), ClientSettings{}) };
        const auto& fire{ frame.Events[static_cast<std::size_t>(InputEvent::Fire)] };
        EXPECT_TRUE(fire.IsDown) << tick;
        EXPECT_EQ(fire.DownCount, tick + 1) << tick;
    }
}

TEST(AgentTickTests, ATapIsDownOnTheFirstTickOnlyAndStaysUpWhenTheWindowRepeats)
{
    AgentClient agent{};
    const AgentDelivery delivery{ MakeDelivery(AgentInput{ .Edge = Bit(AgentControl::Jump) }, 4, 1) };

    for (std::uint32_t tick{ 0 }; tick < 10; ++tick)
    {
        const InputFrame frame{ MakeAgentTickFrame(agent, delivery, AdvanceAgentWindow(agent, 1), ClientSettings{}) };
        EXPECT_EQ(frame.Events[static_cast<std::size_t>(InputEvent::Jump)].IsDown, tick == 0) << tick;
    }
}

TEST(AgentTickTests, ANewDeliveryOfTheSameTapPressesAgain)
{
    AgentClient agent{};
    const AgentInput tap{ .Edge = Bit(AgentControl::Jump) };

    std::vector<bool> isDown{};
    for (std::uint64_t sequence{ 1 }; sequence <= 3; ++sequence)
    {
        const AgentDelivery delivery{ MakeDelivery(tap, 4, sequence) };
        for (std::uint32_t tick{ 0 }; tick < 6; ++tick)
        {
            const InputFrame frame{ MakeAgentTickFrame(agent, delivery, AdvanceAgentWindow(agent, sequence), ClientSettings{}) };
            isDown.push_back(frame.Events[static_cast<std::size_t>(InputEvent::Jump)].IsDown);
        }
    }

    for (std::size_t tick{ 0 }; tick < isDown.size(); ++tick) EXPECT_EQ(isDown[tick], tick % 6 == 0) << tick;
}

TEST(AgentTickTests, TheLookTurnsTheRequestedAnglePerWindowAndNothingAfterIt)
{
    AgentClient agent{};
    const AgentDelivery delivery{ MakeDelivery(AgentInput{ .YawDelta = 0.4f, .PitchDelta = -0.2f }, 4, 1) };
    const UnitClientState unit{ MakeUnit(7) };

    for (std::uint32_t tick{ 0 }; tick < 4; ++tick) RunTick(agent, delivery, unit);
    EXPECT_NEAR(agent.Client.State.Yaw, 1.0f + 0.4f, 1e-5f);
    EXPECT_NEAR(agent.Client.State.Pitch, -0.2f, 1e-5f);

    for (std::uint32_t tick{ 4 }; tick < 9; ++tick) RunTick(agent, delivery, unit);
    EXPECT_NEAR(agent.Client.State.Yaw, 1.0f + 0.4f, 1e-5f);
    EXPECT_NEAR(agent.Client.State.Pitch, -0.2f, 1e-5f);
}

TEST(AgentTickTests, TheTurnLimitCutsTheLookPerTickAndWhatItCutsIsLost)
{
    AgentClient agent{};
    const AgentDelivery delivery{ MakeDelivery(AgentInput{ .YawDelta = 0.4f }, 4, 1, 0.02f) };
    const UnitClientState unit{ MakeUnit(7) };

    for (std::uint32_t tick{ 0 }; tick < 8; ++tick) RunTick(agent, delivery, unit);
    EXPECT_NEAR(agent.Client.State.Yaw, 1.0f + (4 * 0.02f), 1e-5f);
}

TEST(AgentTickTests, TheMoveAxesGoOnPastTheWindowWithTheSignOfLeft)
{
    AgentClient agent{};
    const AgentDelivery delivery{ MakeDelivery(AgentInput{ .MoveForward = 1.0f, .MoveLeft = 1.0f }, 4, 1) };
    const UnitClientState unit{ MakeUnit(7) };

    for (std::uint32_t tick{ 0 }; tick < 9; ++tick)
    {
        const PossessedAction action{ RunTick(agent, delivery, unit) };
        EXPECT_EQ(action.Override.ThrottleForward, 1.0f) << tick;
        EXPECT_EQ(action.Override.ThrottleLeft, 1.0f) << tick;
    }
}

TEST(AgentTickTests, APressedRequestIsDeliveredOnceInTheWholeWindowAndAfter)
{
    AgentClient agent{};
    const AgentDelivery delivery{ MakeDelivery(AgentInput{ .Edge = Bit(AgentControl::SwitchWeapon) | Bit(AgentControl::Reload) }, 4, 1) };
    const UnitClientState unit{ MakeUnit(7) };

    std::uint32_t switches{ 0 };
    std::uint32_t reloads{ 0 };
    for (std::uint32_t tick{ 0 }; tick < 10; ++tick)
    {
        const PossessedAction action{ RunTick(agent, delivery, unit) };
        if (action.Requests.IsWeaponSwitch) ++switches;
        if ((action.Override.ClientEvents & 1u) != 0) ++reloads;
    }

    EXPECT_EQ(switches, 1u);
    EXPECT_EQ(reloads, 1u);
}

TEST(AgentTickTests, AHeldFireGivesTheTriggerEveryTick)
{
    AgentClient agent{};
    const AgentDelivery delivery{ MakeDelivery(AgentInput{ .Level = Bit(AgentControl::Fire), .Edge = Bit(AgentControl::Fire) }, 4, 1) };
    const UnitClientState unit{ MakeUnit(7) };

    for (std::uint32_t tick{ 0 }; tick < 9; ++tick)
    {
        const PossessedAction action{ RunTick(agent, delivery, unit) };
        EXPECT_EQ(action.Override.FireTrigger, 1.0f) << tick;
    }
}

TEST(AgentTickTests, ANewUnitStartsTheAimFromTheUnitAndPressesTheHeldControlsAgain)
{
    AgentClient agent{};
    const AgentDelivery delivery{ MakeDelivery(AgentInput{ .Level = Bit(AgentControl::Fire), .Edge = Bit(AgentControl::Fire) }, 4, 1) };

    RunTick(agent, delivery, MakeUnit(7));
    RunTick(agent, delivery, MakeUnit(7));
    EXPECT_EQ(agent.Input.DownCount[static_cast<std::size_t>(AgentControl::Fire)], 2);

    UnitClientState respawned{ MakeUnit(8) };
    respawned.Facing = { 3.0f, 0.1f };
    RunTick(agent, delivery, respawned);
    EXPECT_EQ(agent.Input.DownCount[static_cast<std::size_t>(AgentControl::Fire)], 1);
    EXPECT_NEAR(agent.Client.State.Yaw, 3.0f, 1e-5f);
    EXPECT_EQ(agent.Client.Unit, 8u);
}

TEST(AgentTickTests, WithoutAUnitTheHeldStateIsForgottenButTheWindowStays)
{
    AgentClient agent{};
    const AgentDelivery delivery{ MakeDelivery(AgentInput{ .Edge = Bit(AgentControl::Jump) }, 4, 1) };

    RunTick(agent, delivery, MakeUnit(7));
    RunTick(agent, delivery, MakeUnit(7));
    ASSERT_EQ(agent.TickInWindow, 1u);

    ResetAgentUnit(agent);
    EXPECT_EQ(agent.Client.Unit, 0xFFFFFFFFu);
    EXPECT_EQ(agent.Input.DownMask, 0u);
    EXPECT_TRUE(agent.HasSequence);
    EXPECT_EQ(agent.TickInWindow, 1u);

    // note: The respawn in the middle of the window does not press the tap again: the sequence is the same, so the tick is 2.
    RunTick(agent, delivery, MakeUnit(9));
    EXPECT_EQ(agent.TickInWindow, 2u);
    EXPECT_EQ(agent.Input.DownMask, 0u);
}

TEST(AgentSettingsTests, TheGameFactsComeFromTheTagAndTheClockAndTheUserPreferencesAreFixed)
{
    std::vector<std::uint8_t> tag(0x100, 0);
    Put<float>(tag, k_HoldThresholdOffset, 0.4f);
    Put<float>(tag, k_ControlTagZoomCancelOffset, 0.7f);
    Put<float>(tag, k_ControlTagJumpTimerOffset, 0.25f);

    std::vector<std::uint8_t> clock(0x40, 0);
    Put<std::int16_t>(clock, k_ClockTicksPerSecondOffset, 30);
    Put<float>(clock, k_ClockTickSecondsOffset, 1.0f / 30.0f);

    std::vector<std::uint8_t> context(0x200, 0);
    Put<std::uintptr_t>(context, k_ClockSlot, reinterpret_cast<std::uintptr_t>(clock.data()));

    const std::optional<ClientSettings> settings{ ReadAgentClientSettings(reinterpret_cast<std::uintptr_t>(tag.data()),
        reinterpret_cast<std::uintptr_t>(context.data())) };
    ASSERT_TRUE(settings.has_value());

    EXPECT_EQ(settings->HoldThresholdMs, 400);
    EXPECT_FLOAT_EQ(settings->ZoomCancelThreshold, 0.7f);
    EXPECT_FLOAT_EQ(settings->JumpTimerSeconds, 0.25f);
    EXPECT_EQ(settings->TicksPerSecond, 30);
    EXPECT_FLOAT_EQ(settings->TickSeconds, 1.0f / 30.0f);

    EXPECT_EQ(settings->AimScaleZoomed, 1.0f);
    EXPECT_EQ(settings->AimScaleSeated, 1.0f);
    EXPECT_FALSE(settings->IsCrouchToggle);
    EXPECT_FALSE(settings->IsJumpFlag);
    EXPECT_FALSE(settings->IsMeleeHoldsUse);
    EXPECT_TRUE(settings->IsGameSystemPresent);
    EXPECT_TRUE(settings->IsSessionReady);
    EXPECT_FALSE(settings->IsMenuContext);
    EXPECT_TRUE(settings->IsInputEnabled);
}

TEST(AgentSettingsTests, WithoutTheTagOrTheClockThereAreNoSettings)
{
    std::vector<std::uint8_t> tag(0x100, 0);
    std::vector<std::uint8_t> context(0x200, 0);

    EXPECT_FALSE(ReadAgentClientSettings(0, reinterpret_cast<std::uintptr_t>(context.data())).has_value());
    EXPECT_FALSE(ReadAgentClientSettings(reinterpret_cast<std::uintptr_t>(tag.data()), reinterpret_cast<std::uintptr_t>(context.data())).has_value());
}