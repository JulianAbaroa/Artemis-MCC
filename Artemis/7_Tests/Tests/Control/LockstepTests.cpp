#include <gtest/gtest.h>

import Common.Math.Type;
import Control.Action.Type;
import Control.Lockstep.Type;
import Control.Lockstep.System;
import Contract.Bridge.Type;
import std;

// Control.Lockstep (M6a, D210-D215): the decisions of the bridge that need no engine and no clock: the control in effect, when the game
// waits for a decision, when a decision is applied, and what the clock of a read tick says. The tests fix what the code does today.

namespace
{
    using Control::Action::Type::ActionOverride;
    using Control::Lockstep::Type::ApplyTiming;
    using Control::Lockstep::Type::BridgeControlState;
    using Control::Lockstep::Type::BridgeHookState;
    using Control::Lockstep::Type::ClockReading;
    using Control::Lockstep::Type::WaitResult;
    using Control::Lockstep::System::ApplyConnection;
    using Control::Lockstep::System::BuildOverride;
    using Control::Lockstep::System::DecideControl;
    using Control::Lockstep::System::GetApplyTiming;
    using Control::Lockstep::System::GetDueDecision;
    using Control::Lockstep::System::GetReleasedOverride;
    using Control::Lockstep::System::IsDecisionTick;
    using Control::Lockstep::System::ReadClock;
    using Control::Lockstep::System::RecordWait;
    using Control::Lockstep::System::ResumeIfAnswered;
    using Control::Lockstep::System::ToClientRequests;

    using Contract::Bridge::Type::BridgeAction;
    using Contract::Bridge::Type::BridgeClientControl;
    using Contract::Bridge::Type::BridgeMode;
    using Contract::Bridge::Type::BridgeOwner;

    using Common::Math::Type::k_HalfPi;
    using Common::Math::Type::k_TwoPi;

    // return: What the client writes in its control block to ask for these.
    auto MakeClient(BridgeOwner owner, BridgeMode mode, std::uint8_t period, std::uint8_t latency, std::uint32_t agentMask)
        -> BridgeClientControl
    {
        BridgeClientControl client{};
        client.RequestOwner = owner;
        client.RequestMode = mode;
        client.RequestPeriod = period;
        client.RequestLatency = latency;
        client.AgentMask = agentMask;
        return client;
    }

    // return: A control in effect for Lockstep with N and d.
    auto MakeLockstep(std::uint8_t period, std::uint8_t latency) -> BridgeControlState
    {
        BridgeControlState control{};
        control.IsClientConnected = true;
        control.Owner = BridgeOwner::Python;
        control.Mode = BridgeMode::Lockstep;
        control.DecisionPeriod = period;
        control.Latency = latency;
        control.AgentMask = 0x3;
        return control;
    }
}

TEST(LockstepTests, DecideControlOwnerModeAndAgents)
{
    const BridgeControlState python{ DecideControl(MakeClient(BridgeOwner::Python, BridgeMode::Lockstep, 0, 0, 0x3), BridgeControlState{}) };
    EXPECT_EQ(python.Owner, BridgeOwner::Python);
    EXPECT_EQ(python.Mode, BridgeMode::Lockstep);
    EXPECT_EQ(python.AgentMask, 0x3u);

    const BridgeControlState ui{ DecideControl(MakeClient(BridgeOwner::UI, BridgeMode::Lockstep, 0, 0, 0x3), BridgeControlState{}) };
    EXPECT_EQ(ui.Owner, BridgeOwner::UI);
    EXPECT_EQ(ui.Mode, BridgeMode::Free);
    EXPECT_EQ(ui.AgentMask, 0u);

    // Python in Free mode keeps its agents.
    const BridgeControlState free{ DecideControl(MakeClient(BridgeOwner::Python, BridgeMode::Free, 0, 0, 0x5), BridgeControlState{}) };
    EXPECT_EQ(free.Owner, BridgeOwner::Python);
    EXPECT_EQ(free.Mode, BridgeMode::Free);
    EXPECT_EQ(free.AgentMask, 0x5u);
}

TEST(LockstepTests, DecideControlClampsPeriodAndLatency)
{
    const BridgeControlState previous{};

    EXPECT_EQ(DecideControl(MakeClient(BridgeOwner::Python, BridgeMode::Free, 100, 0, 0), previous).DecisionPeriod, 60);
    EXPECT_EQ(DecideControl(MakeClient(BridgeOwner::Python, BridgeMode::Free, 1, 0, 0), previous).DecisionPeriod, 2);
    EXPECT_EQ(DecideControl(MakeClient(BridgeOwner::Python, BridgeMode::Free, 0, 1, 0), previous).Latency, 2);

    // d never goes over N.
    const BridgeControlState tight{ DecideControl(MakeClient(BridgeOwner::Python, BridgeMode::Free, 4, 9, 0), previous) };
    EXPECT_EQ(tight.DecisionPeriod, 4);
    EXPECT_EQ(tight.Latency, 4);

    // A request of 0 keeps the previous N and d, and d is cut to the range again.
    BridgeControlState before{};
    before.DecisionPeriod = 8;
    before.Latency = 5;
    const BridgeControlState kept{ DecideControl(MakeClient(BridgeOwner::Python, BridgeMode::Free, 0, 0, 0), before) };
    EXPECT_EQ(kept.DecisionPeriod, 8);
    EXPECT_EQ(kept.Latency, 5);

    before.DecisionPeriod = 3;
    const BridgeControlState cut{ DecideControl(MakeClient(BridgeOwner::Python, BridgeMode::Free, 0, 0, 0), before) };
    EXPECT_EQ(cut.DecisionPeriod, 3);
    EXPECT_EQ(cut.Latency, 3);
}

TEST(LockstepTests, ApplyConnectionReleasesWhenGone)
{
    const BridgeControlState control{ MakeLockstep(6, 3) };

    const BridgeControlState connected{ ApplyConnection(control, true) };
    EXPECT_TRUE(connected.IsClientConnected);
    EXPECT_EQ(connected.Owner, BridgeOwner::Python);
    EXPECT_EQ(connected.Mode, BridgeMode::Lockstep);
    EXPECT_EQ(connected.AgentMask, 0x3u);
    EXPECT_EQ(connected.DecisionPeriod, 6);
    EXPECT_EQ(connected.Latency, 3);

    const BridgeControlState gone{ ApplyConnection(control, false) };
    EXPECT_FALSE(gone.IsClientConnected);
    EXPECT_EQ(gone.Owner, BridgeOwner::UI);
    EXPECT_EQ(gone.Mode, BridgeMode::Free);
    EXPECT_EQ(gone.AgentMask, 0u);
}

TEST(LockstepTests, BuildOverrideSanitizes)
{
    constexpr float nan{ std::numeric_limits<float>::quiet_NaN() };
    constexpr float inf{ std::numeric_limits<float>::infinity() };

    BridgeAction action{};
    action.Buttons = 0x1234;
    action.Lanes = 0x56;
    action.Yaw = -0.5f;
    action.Pitch = 0.25f;
    action.ThrottleForward = 0.5f;
    action.ThrottleLeft = -0.5f;

    bool isRejected{ false };
    ActionOverride result{ BuildOverride(action, 0.2f, isRejected) };
    EXPECT_NEAR(result.Yaw, k_TwoPi - 0.5f, 1e-6);
    EXPECT_FLOAT_EQ(result.Pitch, 0.25f);
    EXPECT_FLOAT_EQ(result.ThrottleForward, 0.5f);
    EXPECT_FLOAT_EQ(result.ThrottleLeft, -0.5f);
    EXPECT_EQ(result.Buttons, 0x1234u);
    EXPECT_EQ(result.Lanes, 0x56u);
    EXPECT_FLOAT_EQ(result.TurnLimit, 0.2f);
    EXPECT_FALSE(result.LocalIndex.has_value());
    EXPECT_FALSE(isRejected);

    // A yaw over a turn wraps without a rejection.
    action.Yaw = 7.0f;
    result = BuildOverride(action, 0.0f, isRejected);
    EXPECT_NEAR(result.Yaw, 7.0f - k_TwoPi, 1e-6);
    EXPECT_FALSE(isRejected);

    // What is not a number or not finite is 0, and counts as a rejection.
    for (const float bad : { nan, inf, -inf })
    {
        BridgeAction broken{};
        broken.Yaw = bad;
        isRejected = false;
        result = BuildOverride(broken, 0.0f, isRejected);
        EXPECT_NEAR(result.Yaw, 0.0f, 1e-6);
        EXPECT_TRUE(isRejected);

        broken = BridgeAction{};
        broken.Pitch = bad;
        isRejected = false;
        result = BuildOverride(broken, 0.0f, isRejected);
        EXPECT_FLOAT_EQ(result.Pitch, 0.0f);
        EXPECT_TRUE(isRejected);

        broken = BridgeAction{};
        broken.ThrottleForward = bad;
        isRejected = false;
        result = BuildOverride(broken, 0.0f, isRejected);
        EXPECT_FLOAT_EQ(result.ThrottleForward, 0.0f);
        EXPECT_TRUE(isRejected);

        broken = BridgeAction{};
        broken.ThrottleLeft = bad;
        isRejected = false;
        result = BuildOverride(broken, 0.0f, isRejected);
        EXPECT_FLOAT_EQ(result.ThrottleLeft, 0.0f);
        EXPECT_TRUE(isRejected);
    }

    // What is out of range is cut to it, and counts as a rejection.
    BridgeAction wide{};
    wide.Pitch = 2.0f;
    isRejected = false;
    result = BuildOverride(wide, 0.0f, isRejected);
    EXPECT_NEAR(result.Pitch, k_HalfPi, 1e-6);
    EXPECT_TRUE(isRejected);

    wide = BridgeAction{};
    wide.ThrottleForward = 1.5f;
    wide.ThrottleLeft = -1.5f;
    isRejected = false;
    result = BuildOverride(wide, 0.0f, isRejected);
    EXPECT_FLOAT_EQ(result.ThrottleForward, 1.0f);
    EXPECT_FLOAT_EQ(result.ThrottleLeft, -1.0f);
    EXPECT_TRUE(isRejected);

    // A turn limit that is not a positive number is 0 (straight to the target), and it is not a rejection.
    for (const float limit : { nan, -1.0f, 0.0f })
    {
        isRejected = false;
        EXPECT_FLOAT_EQ(BuildOverride(BridgeAction{}, limit, isRejected).TurnLimit, 0.0f);
        EXPECT_FALSE(isRejected);
    }

    // The flag is only raised: a clean action leaves it as it came.
    isRejected = true;
    BuildOverride(BridgeAction{}, 0.0f, isRejected);
    EXPECT_TRUE(isRejected);
}

TEST(LockstepTests, ToClientRequestsBits)
{
    auto requests = ToClientRequests(0x1);
    EXPECT_TRUE(requests.IsWeaponSwitch);
    EXPECT_FALSE(requests.IsGrenadeSwitch);
    EXPECT_FALSE(requests.IsZoom);

    requests = ToClientRequests(0x2);
    EXPECT_FALSE(requests.IsWeaponSwitch);
    EXPECT_TRUE(requests.IsGrenadeSwitch);
    EXPECT_FALSE(requests.IsZoom);

    requests = ToClientRequests(0x4);
    EXPECT_FALSE(requests.IsWeaponSwitch);
    EXPECT_FALSE(requests.IsGrenadeSwitch);
    EXPECT_TRUE(requests.IsZoom);

    requests = ToClientRequests(0x7);
    EXPECT_TRUE(requests.IsWeaponSwitch);
    EXPECT_TRUE(requests.IsGrenadeSwitch);
    EXPECT_TRUE(requests.IsZoom);

    requests = ToClientRequests(0x0);
    EXPECT_FALSE(requests.IsWeaponSwitch);
    EXPECT_FALSE(requests.IsGrenadeSwitch);
    EXPECT_FALSE(requests.IsZoom);
}

TEST(LockstepTests, GetDueDecision)
{
    const BridgeControlState control{ MakeLockstep(4, 2) };
    const BridgeHookState hook{};

    // At tick 6 the decision 4 is due (tick 4 + d).
    EXPECT_EQ(GetDueDecision(6, control, hook, 4, 3), 4u);
    EXPECT_EQ(GetDueDecision(6, control, hook, 4, 4), 0u);
    EXPECT_EQ(GetDueDecision(6, control, hook, 3, 3), 0u);

    // Before the first decision tick plus d there is none.
    EXPECT_EQ(GetDueDecision(5, control, hook, 4, 3), 0u);
    EXPECT_EQ(GetDueDecision(1, control, hook, 4, 0), 0u);

    BridgeControlState free{ control };
    free.Mode = BridgeMode::Free;
    EXPECT_EQ(GetDueDecision(6, free, hook, 4, 3), 0u);

    BridgeHookState suspended{};
    suspended.IsLockstepSuspended = true;
    EXPECT_EQ(GetDueDecision(6, control, suspended, 4, 3), 0u);
}

TEST(LockstepTests, RecordWaitSuspendsAfterTwoTimeouts)
{
    BridgeHookState hook{};

    EXPECT_FALSE(RecordWait(hook, WaitResult::TimedOut));
    EXPECT_EQ(hook.TimeoutsInRow, 1u);
    EXPECT_FALSE(hook.IsLockstepSuspended);

    EXPECT_TRUE(RecordWait(hook, WaitResult::TimedOut));
    EXPECT_EQ(hook.TimeoutsInRow, 2u);
    EXPECT_TRUE(hook.IsLockstepSuspended);

    // A wait cut short changes nothing, one answered starts the count again.
    EXPECT_FALSE(RecordWait(hook, WaitResult::Cut));
    EXPECT_EQ(hook.TimeoutsInRow, 2u);
    EXPECT_TRUE(hook.IsLockstepSuspended);

    EXPECT_FALSE(RecordWait(hook, WaitResult::Answered));
    EXPECT_EQ(hook.TimeoutsInRow, 0u);
}

TEST(LockstepTests, ResumeIfAnswered)
{
    BridgeHookState hook{};
    hook.IsLockstepSuspended = true;
    hook.TimeoutsInRow = 2;
    hook.AppliedSequence = 8;

    EXPECT_FALSE(ResumeIfAnswered(hook, 8));
    EXPECT_TRUE(hook.IsLockstepSuspended);

    EXPECT_TRUE(ResumeIfAnswered(hook, 12));
    EXPECT_FALSE(hook.IsLockstepSuspended);
    EXPECT_EQ(hook.TimeoutsInRow, 0u);

    EXPECT_FALSE(ResumeIfAnswered(hook, 20));
}

TEST(LockstepTests, GetApplyTiming)
{
    EXPECT_EQ(GetApplyTiming(4, 0, 2, 6), ApplyTiming::OnTime);
    EXPECT_EQ(GetApplyTiming(4, 0, 2, 7), ApplyTiming::Late);
    EXPECT_EQ(GetApplyTiming(4, 0, 2, 5), ApplyTiming::None);

    // A decision already applied is not applied again.
    EXPECT_EQ(GetApplyTiming(4, 4, 2, 6), ApplyTiming::None);
}

TEST(LockstepTests, GetReleasedOverride)
{
    ActionOverride bot{};
    bot.Yaw = 1.0f;
    bot.Pitch = 0.2f;
    bot.Buttons = 5;
    bot.ThrottleForward = 1.0f;

    const std::optional<ActionOverride> released{ GetReleasedOverride(bot) };
    ASSERT_TRUE(released.has_value());
    EXPECT_FLOAT_EQ(released->Yaw, 1.0f);
    EXPECT_FLOAT_EQ(released->Pitch, 0.2f);
    EXPECT_EQ(released->Buttons, 0u);
    EXPECT_FLOAT_EQ(released->ThrottleForward, 0.0f);
    EXPECT_FLOAT_EQ(released->ThrottleLeft, 0.0f);
    EXPECT_FLOAT_EQ(released->TurnLimit, 0.0f);
    EXPECT_EQ(released->Lanes, 0u);
    EXPECT_FALSE(released->LocalIndex.has_value());

    // A local player goes back to its own input.
    ActionOverride local{ bot };
    local.LocalIndex = 0;
    EXPECT_FALSE(GetReleasedOverride(local).has_value());

    EXPECT_FALSE(GetReleasedOverride(std::nullopt).has_value());
}

TEST(LockstepTests, ReadClock)
{
    // Another match: the clock starts again, never a skip.
    ClockReading reading{ ReadClock(3, 100, 1, 2, true) };
    EXPECT_TRUE(reading.IsNewMatch);
    EXPECT_FALSE(reading.IsRepeated);
    EXPECT_FALSE(reading.IsSkip);

    // The same tick again.
    reading = ReadClock(10, 10, 2, 2, true);
    EXPECT_FALSE(reading.IsNewMatch);
    EXPECT_TRUE(reading.IsRepeated);
    EXPECT_FALSE(reading.IsSkip);

    // Nothing was read before.
    reading = ReadClock(10, 0, 2, 2, true);
    EXPECT_FALSE(reading.IsNewMatch);
    EXPECT_FALSE(reading.IsRepeated);
    EXPECT_FALSE(reading.IsSkip);

    // The next tick.
    reading = ReadClock(11, 10, 2, 2, true);
    EXPECT_FALSE(reading.IsNewMatch);
    EXPECT_FALSE(reading.IsRepeated);
    EXPECT_FALSE(reading.IsSkip);

    // A jump in a run of consecutive reads is a skip, after dropped ticks it is not.
    reading = ReadClock(13, 10, 2, 2, true);
    EXPECT_FALSE(reading.IsNewMatch);
    EXPECT_FALSE(reading.IsRepeated);
    EXPECT_TRUE(reading.IsSkip);

    reading = ReadClock(13, 10, 2, 2, false);
    EXPECT_FALSE(reading.IsSkip);
}

TEST(LockstepTests, IsDecisionTick)
{
    const BridgeControlState control{ MakeLockstep(4, 2) };

    EXPECT_TRUE(IsDecisionTick(true, control, 8, false));
    EXPECT_FALSE(IsDecisionTick(true, control, 9, false));
    EXPECT_FALSE(IsDecisionTick(true, control, 0, false));
    EXPECT_FALSE(IsDecisionTick(true, control, 8, true));
    EXPECT_FALSE(IsDecisionTick(false, control, 8, false));

    BridgeControlState gone{ control };
    gone.IsClientConnected = false;
    EXPECT_FALSE(IsDecisionTick(true, gone, 8, false));
}