module Control.Lockstep.System;

import Common.Math.Type;
import Control.Action.Type;
import Control.Lockstep.Type;
import Contract.Bridge.Type;
import std;

namespace
{
    using Control::Action::Type::ActionOverride;
    using Control::Action::Type::ClientRequests;
    using Control::Lockstep::Type::ApplyTiming;
    using Control::Lockstep::Type::BridgeControlState;
    using Control::Lockstep::Type::BridgeHookState;
    using Control::Lockstep::Type::ClockReading;
    using Control::Lockstep::Type::WaitResult;

    using Contract::Bridge::Type::BridgeAction;
    using Contract::Bridge::Type::BridgeClientControl;
    using Contract::Bridge::Type::BridgeMode;
    using Contract::Bridge::Type::BridgeOwner;
    using Contract::Bridge::Type::BridgeRequest;

    using Common::Math::Type::k_TwoPi;
    using Common::Math::Type::k_HalfPi;
    using Control::Lockstep::Type::k_MaxLockstepTimeouts;
    using Control::Lockstep::Type::k_MaxDecisionPeriod;
    using Control::Lockstep::Type::k_MinLatency;

    // return: The value, or 0 when it is not finite. Sets isRejected when it had to change it.
    auto Finite(float value, bool& isRejected) -> float
    {
        if (std::isfinite(value)) return value;
        isRejected = true;
        return 0.0f;
    }

    auto Clamp(float value, float limit, bool& isRejected) -> float
    {
        if (value >= -limit && value <= limit) return value;
        isRejected = true;
        return std::clamp(value, -limit, limit);
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
        }

        return control;
    }

    auto BuildOverride(const BridgeAction& action, float turnLimit, bool& isRejected) -> ActionOverride
    {
        // The engine keeps the yaw in [0, 2 pi): any finite yaw is wrapped there.
        float yaw{ std::fmod(Finite(action.Yaw, isRejected), k_TwoPi) };
        if (yaw < 0.0f) yaw += k_TwoPi;

        ActionOverride actionOverride{};
        actionOverride.Buttons = action.Buttons;
        actionOverride.Yaw = yaw;
        actionOverride.Pitch = Clamp(Finite(action.Pitch, isRejected), k_HalfPi, isRejected);
        actionOverride.ThrottleForward = Clamp(Finite(action.ThrottleForward, isRejected), 1.0f, isRejected);
        actionOverride.ThrottleLeft = Clamp(Finite(action.ThrottleLeft, isRejected), 1.0f, isRejected);
        actionOverride.Lanes = action.Lanes;
        actionOverride.TurnLimit = std::isfinite(turnLimit) && turnLimit > 0.0f ? turnLimit : 0.0f;

        return actionOverride;
    }

    auto ToClientRequests(std::uint8_t requests) -> ClientRequests
    {
        return ClientRequests{
            .IsWeaponSwitch = (requests & static_cast<std::uint8_t>(BridgeRequest::WeaponSwitch)) != 0,
            .IsGrenadeSwitch = (requests & static_cast<std::uint8_t>(BridgeRequest::GrenadeSwitch)) != 0,
            .IsZoom = (requests & static_cast<std::uint8_t>(BridgeRequest::Zoom)) != 0 };
    }

    auto GetDueDecision(std::uint64_t actionTick, const BridgeControlState& control, const BridgeHookState& hook,
        std::uint64_t pushedDecision, std::uint64_t sequence) -> std::uint64_t
    {
        if (control.Mode != BridgeMode::Lockstep || hook.IsLockstepSuspended || actionTick < control.Latency) return 0;

        // note: The AI thread hands decision t over before it reads tick t + 1, and the game thread waits for that read, so at tick
        // t + d (d >= 2) a decision that will be published is always pushed already.
        const std::uint64_t due{ (actionTick - control.Latency) / control.DecisionPeriod * control.DecisionPeriod };
        if (due > 0 && due <= pushedDecision && sequence < due) return due;

        return 0;
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

    auto GetReleasedOverride(const std::optional<ActionOverride>& last) -> std::optional<ActionOverride>
    {
        if (last.has_value() && !last->LocalIndex.has_value()) return ActionOverride{ .Yaw = last->Yaw, .Pitch = last->Pitch };

        return std::nullopt;
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