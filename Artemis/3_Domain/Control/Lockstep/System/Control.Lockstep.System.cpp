module Control.Lockstep.System;

import Control.Client.Type;
import Control.Client.System;
import Control.Lockstep.Type;
import Contract.Bridge.Type;
import std;

namespace
{
    using Control::Client::System::SanitizeAgentInput;
    using Control::Client::Type::AgentInput;
    using Control::Lockstep::Type::ApplyTiming;
    using Control::Lockstep::Type::BridgeControlState;
    using Control::Lockstep::Type::BridgeHookState;
    using Control::Lockstep::Type::ClockReading;
    using Control::Lockstep::Type::WaitResult;

    using Contract::Bridge::Type::BridgeAgentInput;
    using Contract::Bridge::Type::BridgeClientControl;
    using Contract::Bridge::Type::BridgeMode;
    using Contract::Bridge::Type::BridgeOwner;

    using Control::Lockstep::Type::k_MaxLockstepTimeouts;
    using Control::Lockstep::Type::k_MaxDecisionPeriod;
    using Control::Lockstep::Type::k_MinLatency;

    // return: Whether the sanitizer left the value as it was. NaN is never equal, so it counts as changed.
    auto IsSame(float value, float clean) -> bool
    {
        return value == clean;
    }
}

namespace Control::Lockstep::System
{
    auto DecideControl(const BridgeClientControl& client, const BridgeControlState& previous) -> BridgeControlState
    {
        BridgeControlState control{ previous };

        control.Owner = client.RequestOwner == BridgeOwner::Python ? BridgeOwner::Python : BridgeOwner::UI;
        control.Mode = control.Owner == BridgeOwner::Python && client.RequestMode == BridgeMode::Lockstep
            ? BridgeMode::Lockstep : BridgeMode::Free;

        if (client.RequestPeriod != 0) control.DecisionPeriod = std::clamp(client.RequestPeriod, k_MinLatency, k_MaxDecisionPeriod);
        if (client.RequestLatency != 0) control.Latency = client.RequestLatency;
        control.Latency = std::clamp(control.Latency, k_MinLatency, control.DecisionPeriod);

        control.AgentMask = control.Owner == BridgeOwner::Python ? client.AgentMask : 0u;
        control.IsSparseRequested = client.RequestSparse != 0;
        return control;
    }

    auto ApplyConnection(BridgeControlState control, bool isConnected) -> BridgeControlState
    {
        control.IsClientConnected = isConnected;
        if (!control.IsClientConnected)
        {
            control.Owner = BridgeOwner::UI;
            control.Mode = BridgeMode::Free;
            control.AgentMask = 0;
            control.IsSparseRequested = false;
        }

        return control;
    }

    auto IsSparse(const BridgeControlState& control, std::uint32_t everyTickDemand) -> bool
    {
        return control.IsClientConnected && control.IsSparseRequested && everyTickDemand == 0;
    }

    auto IsLightTick(bool isSparse, bool isDecision, bool wasSparse, bool isFirstAfterLoad) -> bool
    {
        return isSparse && !isDecision && wasSparse && !isFirstAfterLoad;
    }

    auto ToAgentInput(const BridgeAgentInput& input, bool& isRejected) -> AgentInput
    {
        const AgentInput clean{ SanitizeAgentInput(AgentInput{ .MoveForward = input.MoveForward, .MoveLeft = input.MoveLeft,
            .YawDelta = input.YawDelta, .PitchDelta = input.PitchDelta, .Level = input.Level, .Edge = input.Edge }) };

        if (!IsSame(input.MoveForward, clean.MoveForward) || !IsSame(input.MoveLeft, clean.MoveLeft) || !IsSame(input.YawDelta, clean.YawDelta)
            || !IsSame(input.PitchDelta, clean.PitchDelta) || input.Level != clean.Level || input.Edge != clean.Edge)
        {
            isRejected = true;
        }

        return clean;
    }

    auto ToBridgeAgentInput(const AgentInput& input) -> BridgeAgentInput
    {
        return BridgeAgentInput{ .MoveForward = input.MoveForward, .MoveLeft = input.MoveLeft, .YawDelta = input.YawDelta,
            .PitchDelta = input.PitchDelta, .Level = input.Level, .Edge = input.Edge };
    }

    auto IsImitationWindowEnd(std::uint64_t actionTick, std::uint8_t period, std::uint8_t latency) -> bool
    {
        if (actionTick == 0 || period == 0 || latency == 0) return false;
        return actionTick % period % latency == 0;
    }

    auto GetDueDecision(std::uint64_t actionTick, const BridgeControlState& control, const BridgeHookState& hook,
        std::uint64_t pushedDecision, std::uint64_t sequence) -> std::uint64_t
    {
        if (control.Mode != BridgeMode::Lockstep || hook.IsLockstepSuspended || actionTick < control.Latency) return 0;

        // note: The game thread marks decision t as pushed when it queues the tick for the AI thread, so at tick t + d a decision that will
        // be published is always marked, however far behind the AI thread is (D305). The wait is then for the answer of the client.
        const std::uint64_t due{ (actionTick - control.Latency) / control.DecisionPeriod * control.DecisionPeriod };
        if (due > 0 && due <= pushedDecision && sequence < due) return due;

        return 0;
    }

    auto GetNeedDecision(std::uint64_t actionTick, const BridgeControlState& control, const BridgeHookState& hook,
        std::uint64_t pushedDecision) -> std::uint64_t
    {
        if (control.Mode != BridgeMode::Lockstep || hook.IsLockstepSuspended || actionTick < control.Latency) return 0;

        const std::uint64_t decision{ (actionTick - control.Latency) / control.DecisionPeriod * control.DecisionPeriod };
        if (decision == 0 || decision > pushedDecision || actionTick != decision + control.Latency) return 0;

        return decision == hook.NeedDecision ? 0 : decision;
    }

    auto RecordWait(BridgeHookState& hook, WaitResult result) -> bool
    {
        switch (result)
        {
        case WaitResult::Answered:
            hook.TimeoutsInRow = 0;
            return false;
        case WaitResult::Cut:
            return false;
        case WaitResult::TimedOut:
            if (++hook.TimeoutsInRow >= k_MaxLockstepTimeouts)
            {
                hook.IsLockstepSuspended = true;
                return true;
            }
            return false;
        }

        return false;
    }

    auto ResumeIfAnswered(BridgeHookState& hook, std::uint64_t sequence) -> bool
    {
        if (!hook.IsLockstepSuspended || sequence <= hook.AppliedSequence) return false;

        hook.IsLockstepSuspended = false;
        hook.TimeoutsInRow = 0;
        return true;
    }

    auto GetApplyTiming(std::uint64_t sequence, std::uint64_t appliedSequence, std::uint8_t latency, std::uint64_t actionTick)
        -> ApplyTiming
    {
        if (sequence <= appliedSequence || sequence + latency > actionTick) return ApplyTiming::None;

        return sequence + latency < actionTick ? ApplyTiming::Late : ApplyTiming::OnTime;
    }

    auto ReadClock(std::uint64_t actionTick, std::uint64_t lastReadTick, std::uint64_t lastReadMatch, std::uint64_t match,
        bool isConsecutive) -> ClockReading
    {
        // note: Every engine starts its clock again (D139): the first read of a new match is not a skip. Neither is a tick where the
        // engine applied no actions (the action tick repeats, seen when a match ends).
        const bool isNewMatch{ match != lastReadMatch };
        const bool isRepeated{ !isNewMatch && lastReadTick != 0 && actionTick == lastReadTick };
        const bool isSkip{ !isNewMatch && isConsecutive && lastReadTick != 0 && !isRepeated && actionTick != lastReadTick + 1 };
        return ClockReading{ .IsNewMatch = isNewMatch, .IsRepeated = isRepeated, .IsSkip = isSkip };
    }

    auto IsDecisionTick(bool isOpen, const BridgeControlState& control, std::uint64_t actionTick, bool isRepeated) -> bool
    {
        return isOpen && control.IsClientConnected && actionTick != 0 && !isRepeated && actionTick % control.DecisionPeriod == 0;
    }
}