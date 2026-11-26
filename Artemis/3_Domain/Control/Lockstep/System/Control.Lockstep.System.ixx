export module Control.Lockstep.System;

import Control.Client.Type;
import Control.Lockstep.Type;
import Contract.Bridge.Type;
import std;

namespace
{
    using Control::Client::Type::AgentInput;
    using Control::Lockstep::Type::ApplyTiming;
    using Control::Lockstep::Type::BridgeControlState;
    using Control::Lockstep::Type::BridgeHookState;
    using Control::Lockstep::Type::ClockReading;
    using Control::Lockstep::Type::HoldAction;
    using Control::Lockstep::Type::WaitResult;

    using Contract::Bridge::Type::BridgeAgentInput;
    using Contract::Bridge::Type::BridgeClientControl;
}

export namespace Control::Lockstep::System
{
    // return: The control in effect for the requests the client wrote, from the one in effect before.
    // note: Python owns the instance only if it asks for it, and Lockstep needs Python. N is kept between k_MinLatency and its maximum
    // and d between k_MinLatency and N. A request of 0 keeps the previous N or d (D106, D114).
    auto DecideControl(const BridgeClientControl& client, const BridgeControlState& previous) -> BridgeControlState;

    // return: The control with the connection of the client. Without it the UI owns the instance, Free, with no agents.
    auto ApplyConnection(BridgeControlState control, bool isConnected) -> BridgeControlState;

    // return: Whether the pipeline runs only on decision ticks (Q101): the client asked for it, it is connected and nobody needs every tick.
    // param everyTickDemand: Recorders and tools that need every tick of the engine (Services::EveryTick), 0 = none.
    // note: Without a client, or when it leaves, every tick runs the pipeline.
    auto IsSparse(const BridgeControlState& control, std::uint32_t everyTickDemand) -> bool;

    // return: Whether a tick only needs the light capture and the flag pass (Q101): sparse mode, not a decision, the previous tick was
    // already sparse and it is not the first tick after a map load. The first tick after a load and the first after the mode turned on run
    // the whole pipeline, so the tick of a decision always has the state it needs.
    auto IsLightTick(bool isSparse, bool isDecision, bool wasSparse, bool isFirstAfterLoad) -> bool;

    // return: The AgentInput the client wrote for a player, sane: the one place that makes the values of the bridge acceptable (D365).
    // param isRejected: Set to true when a value had to be changed. Never set to false.
    // note: The move axes are cut to [-1, 1], a value that is not finite is 0 (also in the look) and the bits of Level and Edge that are no control
    // are dropped (SanitizeAgentInput). The turn limit is not here: it travels in the control block and the store keeps it sane.
    auto ToAgentInput(const BridgeAgentInput& input, bool& isRejected) -> AgentInput;

    // return: The AgentInput as the bridge writes it (the imitation block of the tick). Copies the values.
    auto ToBridgeAgentInput(const AgentInput& input) -> BridgeAgentInput;

    // return: Whether the window of the possessed player closes at this action tick (D364): windows of d ticks counted from the last decision tick
    // (a multiple of N), so the last one of each period ends where the next decision begins.
    // param period: N, the decision period. 0 never closes.
    // param latency: d, the ticks of a window. 0 never closes.
    auto IsImitationWindowEnd(std::uint64_t actionTick, std::uint8_t period, std::uint8_t latency) -> bool;

    // return: The decision the game thread has to wait for before this action tick, or 0 if it does not wait.
    // param pushedDecision: Last decision tick handed to the AI thread, so its frame will reach the client (D305).
    // param sequence: Last decision the client answered.
    // note: Only in Lockstep, not suspended. The tick of a decision plus the latency waits for it, if it was published.
    auto GetDueDecision(std::uint64_t actionTick, const BridgeControlState& control, const BridgeHookState& hook,
        std::uint64_t pushedDecision, std::uint64_t sequence) -> std::uint64_t;

    // return: The decision the game thread reaches at this action tick in Lockstep (the tick of the decision plus the latency), whether or not
    // its answer is already there, or 0 when this tick is not one of those.
    // param pushedDecision: Last decision tick handed to the AI thread, as in GetDueDecision.
    // note: Only in Lockstep, not suspended, and once per decision: a repeated action tick, or the ticks between t + d and the next decision
    // when d is below N, give 0. A decision already recorded in the hook (NeedDecision) gives 0.
    auto GetNeedDecision(std::uint64_t actionTick, const BridgeControlState& control, const BridgeHookState& hook,
        std::uint64_t pushedDecision) -> std::uint64_t;

    // return: What the tick gate does before a tick that would wait for a decision (TG.1, D380). The order of the checks is the rule:
    // answered runs, a stopped bridge or a client that is gone runs (the PlayerAction hook cuts its wait at once), a pause holds, a cap that
    // passed runs (the PlayerAction hook counts the timeout without waiting again), a budget that passed holds, otherwise it waits.
    // param isCapPassed: The time cap of the wait (k_LockstepTimeout) passed.
    // param isBudgetPassed: The time the gate may hold the game thread in one frame passed.
    auto DecideHold(bool isAnswered, bool isClientGone, bool isStopped, bool isPaused, bool isCapPassed, bool isBudgetPassed) -> HoldAction;

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

    // return: What the clock of this read says about the last read tick.
    // param isConsecutive: False after dropped ticks or a reset, so a jump is not a skip.
    auto ReadClock(std::uint64_t actionTick, std::uint64_t lastReadTick, std::uint64_t lastReadMatch, std::uint64_t match,
        bool isConsecutive) -> ClockReading;

    // return: True if this tick is a decision tick: the bridge is open, the client is connected, the action tick is new and a
    // multiple of N.
    auto IsDecisionTick(bool isOpen, const BridgeControlState& control, std::uint64_t actionTick, bool isRepeated) -> bool;
}