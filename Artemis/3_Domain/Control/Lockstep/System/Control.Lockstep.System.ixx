export module Control.Lockstep.System;

import Control.Action.Type;
import Control.Lockstep.Type;
import Contract.Bridge;
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

    using Platform::Bridge::Type::BridgeAction;
    using Platform::Bridge::Type::BridgeClientControl;
}

export namespace Control::Lockstep::System
{
    // return: The control in effect for the requests the client wrote, from the one in effect before.
    // note: Python owns the instance only if it asks for it, and Lockstep needs Python. N is kept between k_MinLatency and its maximum
    // and d between k_MinLatency and N. A request of 0 keeps the previous N or d (D106, D114).
    auto DecideControl(const BridgeClientControl& client, const BridgeControlState& previous) -> BridgeControlState;

    // return: The control with the connection of the client. Without it the UI owns the instance, Free, with no agents.
    auto ApplyConnection(BridgeControlState control, bool isConnected) -> BridgeControlState;

    // return: The override of a player for the action of the client, with the values the engine accepts.
    // param turnLimit: Turn limit of the client. A value that is not a positive number is 0 (straight to the target).
    // param isRejected: Set to true when a value had to be changed. Never set to false.
    // note: The yaw is wrapped to [0, 2 pi). Pitch and throttle are cut to their range. A value that is not finite is 0.
    auto BuildOverride(const BridgeAction& action, float turnLimit, bool& isRejected) -> ActionOverride;

    // return: The one-tick presses of the BridgeRequest bits of an action.
    auto ToClientRequests(std::uint8_t requests) -> ClientRequests;

    // return: The decision the game thread has to wait for before this action tick, or 0 if it does not wait.
    // param pushedDecision: Last decision the AI thread handed to the bridge.
    // param sequence: Last decision the client answered.
    // note: Only in Lockstep, not suspended. The tick of a decision plus the latency waits for it, if it was published.
    auto GetDueDecision(std::uint64_t actionTick, const BridgeControlState& control, const BridgeHookState& hook,
        std::uint64_t pushedDecision, std::uint64_t sequence) -> std::uint64_t;

    // Records how a wait ended in the state of the hook.
    // return: True only when this wait suspended Lockstep, so the caller logs it.
    auto RecordWait(BridgeHookState& hook, WaitResult result) -> bool;

    // Resumes a suspended Lockstep when the client answered a decision newer than the last applied.
    // return: True when it resumed, so the caller logs it.
    auto ResumeIfAnswered(BridgeHookState& hook, std::uint64_t sequence) -> bool;

    // return: Whether the decision `sequence` is applied in this action tick. A decision is applied at its tick plus the latency, or
    // as soon as it arrives when it is late.
    auto GetApplyTiming(std::uint64_t sequence, std::uint64_t appliedSequence, std::uint8_t latency, std::uint64_t actionTick)
        -> ApplyTiming;

    // return: The override of a player that leaves the client. A bot stands still with the facing it had, and a local player goes back
    // to its own input (nullopt).
    auto GetReleasedOverride(const std::optional<ActionOverride>& last) -> std::optional<ActionOverride>;

    // return: What the clock of this read says about the last read tick.
    // param isConsecutive: False after dropped ticks or a reset, so a jump is not a skip.
    auto ReadClock(std::uint64_t actionTick, std::uint64_t lastReadTick, std::uint64_t lastReadMatch, std::uint64_t match,
        bool isConsecutive) -> ClockReading;

    // return: True if this tick is a decision tick: the bridge is open, the client is connected, the action tick is new and a
    // multiple of N.
    auto IsDecisionTick(bool isOpen, const BridgeControlState& control, std::uint64_t actionTick, bool isRepeated) -> bool;
}