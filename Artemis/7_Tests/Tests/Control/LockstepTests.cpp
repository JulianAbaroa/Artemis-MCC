#include <gtest/gtest.h>

import Control.Client.Type;
import Control.Lockstep.Type;
import Control.Lockstep.System;
import Contract.Bridge.Type;
import std;

// Control.Lockstep (M6a, D210-D215): the decisions of the bridge that need no engine and no clock: the control in effect, when the game
// waits for a decision, when a decision is applied, and what the clock of a read tick says. The tests fix what the code does today.

namespace
{
    using Control::Client::Type::AgentInput;
    using Control::Lockstep::Type::ApplyTiming;
    using Control::Lockstep::Type::BridgeControlState;
    using Control::Lockstep::Type::BridgeHookState;
    using Control::Lockstep::Type::ClockReading;
    using Control::Lockstep::Type::k_DefaultLatency;
    using Control::Lockstep::Type::k_MinLatency;
    using Control::Lockstep::Type::WaitResult;
    using Control::Lockstep::System::ApplyConnection;
    using Control::Lockstep::System::DecideControl;
    using Control::Lockstep::System::GetApplyTiming;
    using Control::Lockstep::System::GetDueDecision;
    using Control::Lockstep::System::GetNeedDecision;
    using Control::Lockstep::System::IsDecisionTick;
    using Control::Lockstep::System::IsImitationWindowEnd;
    using Control::Lockstep::System::IsLightTick;
    using Control::Lockstep::System::IsSparse;
    using Control::Lockstep::System::ReadClock;
    using Control::Lockstep::System::RecordWait;
    using Control::Lockstep::System::ResumeIfAnswered;
    using Control::Lockstep::System::ToAgentInput;
    using Control::Lockstep::System::ToBridgeAgentInput;

    using Contract::Bridge::Type::BridgeAgentInput;
    using Contract::Bridge::Type::BridgeClientControl;
    using Contract::Bridge::Type::BridgeMode;
    using Contract::Bridge::Type::BridgeOwner;

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

    // D304: d = 4 until the client asks for another one; k_MinLatency stays the floor.
    EXPECT_EQ(previous.Latency, 4);
    EXPECT_EQ(k_DefaultLatency, 4);
    EXPECT_EQ(k_MinLatency, 2);
    EXPECT_EQ(DecideControl(MakeClient(BridgeOwner::Python, BridgeMode::Free, 0, 0, 0), previous).Latency, 4);
    EXPECT_EQ(DecideControl(MakeClient(BridgeOwner::Python, BridgeMode::Free, 0, 2, 0), previous).Latency, 2);

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

TEST(LockstepTests, ToAgentInputKeepsASaneInputAsItIs)
{
    const BridgeAgentInput input{ .MoveForward = 1.0f, .MoveLeft = -0.25f, .YawDelta = 3.0f, .PitchDelta = -0.5f, .Level = 0x1FFFu, .Edge = 0x0A5u };

    bool isRejected{ false };
    const AgentInput clean{ ToAgentInput(input, isRejected) };
    EXPECT_FALSE(isRejected);
    EXPECT_FLOAT_EQ(clean.MoveForward, 1.0f);
    EXPECT_FLOAT_EQ(clean.MoveLeft, -0.25f);
    EXPECT_FLOAT_EQ(clean.YawDelta, 3.0f);
    EXPECT_FLOAT_EQ(clean.PitchDelta, -0.5f);
    EXPECT_EQ(clean.Level, 0x1FFFu);
    EXPECT_EQ(clean.Edge, 0x0A5u);

    // The way back gives the same values.
    const BridgeAgentInput back{ ToBridgeAgentInput(clean) };
    EXPECT_FLOAT_EQ(back.MoveForward, input.MoveForward);
    EXPECT_FLOAT_EQ(back.MoveLeft, input.MoveLeft);
    EXPECT_FLOAT_EQ(back.YawDelta, input.YawDelta);
    EXPECT_FLOAT_EQ(back.PitchDelta, input.PitchDelta);
    EXPECT_EQ(back.Level, input.Level);
    EXPECT_EQ(back.Edge, input.Edge);
}

TEST(LockstepTests, ToAgentInputSanitizes)
{
    constexpr float nan{ std::numeric_limits<float>::quiet_NaN() };
    constexpr float inf{ std::numeric_limits<float>::infinity() };

    // Each value that has to change raises the flag on its own.
    for (int field{ 0 }; field < 6; ++field)
    {
        BridgeAgentInput input{};
        switch (field)
        {
        case 0: input.MoveForward = 2.0f; break;
        case 1: input.MoveLeft = -inf; break;
        case 2: input.YawDelta = nan; break;
        case 3: input.PitchDelta = inf; break;
        case 4: input.Level = 1u << 13; break;
        default: input.Edge = 0x8000'0000u; break;
        }

        bool isRejected{ false };
        const AgentInput clean{ ToAgentInput(input, isRejected) };
        EXPECT_TRUE(isRejected) << field;
        EXPECT_FLOAT_EQ(clean.MoveForward, field == 0 ? 1.0f : 0.0f) << field;
        EXPECT_FLOAT_EQ(clean.MoveLeft, 0.0f) << field;
        EXPECT_FLOAT_EQ(clean.YawDelta, 0.0f) << field;
        EXPECT_FLOAT_EQ(clean.PitchDelta, 0.0f) << field;
        EXPECT_EQ(clean.Level, 0u) << field;
        EXPECT_EQ(clean.Edge, 0u) << field;
    }

    // The bits 13 to 31 are dropped and the control bits stay.
    bool isRejected{ false };
    const AgentInput clean{ ToAgentInput(BridgeAgentInput{ .Level = 0xFFFF'FFFFu, .Edge = 0xFFFF'E001u }, isRejected) };
    EXPECT_TRUE(isRejected);
    EXPECT_EQ(clean.Level, 0x1FFFu);
    EXPECT_EQ(clean.Edge, 0x0001u);

    // The flag is only raised: a clean input leaves it as it came.
    isRejected = true;
    ToAgentInput(BridgeAgentInput{}, isRejected);
    EXPECT_TRUE(isRejected);
}

TEST(LockstepTests, IsImitationWindowEnd)
{
    // N 4 and d 2: the windows are [4k, 4k + 2) and [4k + 2, 4k + 4): they end at the even ticks, the last of each period at the decision tick.
    EXPECT_FALSE(IsImitationWindowEnd(1, 4, 2));
    EXPECT_TRUE(IsImitationWindowEnd(2, 4, 2));
    EXPECT_FALSE(IsImitationWindowEnd(3, 4, 2));
    EXPECT_TRUE(IsImitationWindowEnd(4, 4, 2));
    EXPECT_TRUE(IsImitationWindowEnd(8, 4, 2));

    // d = N: one window per period.
    EXPECT_FALSE(IsImitationWindowEnd(2, 4, 4));
    EXPECT_TRUE(IsImitationWindowEnd(4, 4, 4));

    // d does not divide N: the count restarts at each decision tick.
    EXPECT_TRUE(IsImitationWindowEnd(3, 5, 3));
    EXPECT_FALSE(IsImitationWindowEnd(4, 5, 3));
    EXPECT_TRUE(IsImitationWindowEnd(5, 5, 3));
    EXPECT_TRUE(IsImitationWindowEnd(8, 5, 3));
    EXPECT_FALSE(IsImitationWindowEnd(9, 5, 3));

    // Nothing closes at tick 0 or with a zero period or window.
    EXPECT_FALSE(IsImitationWindowEnd(0, 4, 2));
    EXPECT_FALSE(IsImitationWindowEnd(4, 0, 2));
    EXPECT_FALSE(IsImitationWindowEnd(4, 4, 0));
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

// The tick t + d of a decision, with or without its answer: the arrival the timeline of the tick measures (P.3 C).
TEST(LockstepTests, GetNeedDecision)
{
    const BridgeControlState control{ MakeLockstep(4, 2) };
    const BridgeHookState hook{};

    // Tick 6 is decision 4 plus d, whether or not the client answered: the answer is not an input.
    EXPECT_EQ(GetNeedDecision(6, control, hook, 4), 4u);
    EXPECT_EQ(GetNeedDecision(10, control, hook, 8), 8u);

    // Not the tick of the decision plus d: before it, after it (d below N) and the decision tick itself.
    EXPECT_EQ(GetNeedDecision(5, control, hook, 4), 0u);
    EXPECT_EQ(GetNeedDecision(7, control, hook, 4), 0u);
    EXPECT_EQ(GetNeedDecision(8, control, hook, 8), 0u);
    EXPECT_EQ(GetNeedDecision(2, control, hook, 4), 0u);
    EXPECT_EQ(GetNeedDecision(1, control, hook, 4), 0u);

    // A decision that was not handed to the AI thread is not needed.
    EXPECT_EQ(GetNeedDecision(6, control, hook, 3), 0u);

    // d equal to N: the tick of the next decision is the one that needs the previous.
    const BridgeControlState equal{ MakeLockstep(4, 4) };
    EXPECT_EQ(GetNeedDecision(8, equal, hook, 4), 4u);
    EXPECT_EQ(GetNeedDecision(7, equal, hook, 4), 0u);

    // Once per decision: a repeated action tick gives 0 after it was recorded, the next decision is a new one.
    BridgeHookState seen{};
    seen.NeedDecision = 4;
    EXPECT_EQ(GetNeedDecision(6, control, seen, 4), 0u);
    EXPECT_EQ(GetNeedDecision(10, control, seen, 8), 8u);

    BridgeControlState free{ control };
    free.Mode = BridgeMode::Free;
    EXPECT_EQ(GetNeedDecision(6, free, hook, 4), 0u);

    BridgeHookState suspended{};
    suspended.IsLockstepSuspended = true;
    EXPECT_EQ(GetNeedDecision(6, control, suspended, 4), 0u);
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

TEST(LockstepTests, SparseNeedsRequestClientAndNoDemand)
{
    BridgeClientControl client{ MakeClient(BridgeOwner::Python, BridgeMode::Lockstep, 4, 2, 0x3) };
    BridgeControlState control{ ApplyConnection(DecideControl(client, BridgeControlState{}), true) };

    // Not asked for: every tick.
    EXPECT_FALSE(control.IsSparseRequested);
    EXPECT_FALSE(IsSparse(control, 0));

    client.RequestSparse = 1;
    control = ApplyConnection(DecideControl(client, control), true);
    EXPECT_TRUE(control.IsSparseRequested);
    EXPECT_TRUE(IsSparse(control, 0));

    // A recorder or a tool that needs every tick wins, and sparse comes back when it is done.
    EXPECT_FALSE(IsSparse(control, 1));
    EXPECT_FALSE(IsSparse(control, 3));
    EXPECT_TRUE(IsSparse(control, 0));

    // Asking for 0 again goes back to every tick.
    client.RequestSparse = 0;
    EXPECT_FALSE(IsSparse(DecideControl(client, control), 0));
}

TEST(LockstepTests, OnlyASparseTickThatIsNotADecisionNorTheFirstOneIsLight)
{
    // IsLightTick(isSparse, isDecision, wasSparse, isFirstAfterLoad)
    EXPECT_TRUE(IsLightTick(true, false, true, false));

    // Not sparse: every tick runs the pipeline.
    EXPECT_FALSE(IsLightTick(false, false, true, false));
    // A decision tick always runs the whole pipeline.
    EXPECT_FALSE(IsLightTick(true, true, true, false));
    // The first tick after the mode turned on runs it too.
    EXPECT_FALSE(IsLightTick(true, false, false, false));
    // And so does the first tick after a map load.
    EXPECT_FALSE(IsLightTick(true, false, true, true));
}

TEST(LockstepTests, SparseEndsWithTheClient)
{
    BridgeClientControl client{ MakeClient(BridgeOwner::Python, BridgeMode::Lockstep, 4, 2, 0x3) };
    client.RequestSparse = 1;
    const BridgeControlState control{ DecideControl(client, BridgeControlState{}) };

    // The request is read before the connection: a client that is not connected never gets sparse.
    EXPECT_FALSE(IsSparse(ApplyConnection(control, false), 0));
    EXPECT_FALSE(ApplyConnection(control, false).IsSparseRequested);
    EXPECT_TRUE(IsSparse(ApplyConnection(control, true), 0));

    // On disconnect the request is gone: a client that comes back asks again.
    EXPECT_FALSE(IsSparse(ApplyConnection(ApplyConnection(control, false), true), 0));
}