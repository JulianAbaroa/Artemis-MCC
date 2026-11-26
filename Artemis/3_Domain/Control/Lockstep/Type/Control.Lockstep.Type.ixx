export module Control.Lockstep.Type;

import Contract.Bridge.Type;
import std;

namespace
{
    using Contract::Bridge::Type::BridgeOwner;
    using Contract::Bridge::Type::BridgeMode;
}

export namespace Control::Lockstep::Type
{
    // Lockstep timeouts in a row after which Lockstep is suspended until the client answers again.
    inline constexpr std::uint32_t k_MaxLockstepTimeouts{ 2 };

    // Decision period N and latency d until the client asks for others (D114). Since D304 d = N, and since D375 N = d = 5: the decision tick and the client get the whole cycle.
    inline constexpr std::uint8_t k_DefaultDecisionPeriod{ 5 };
    inline constexpr std::uint8_t k_DefaultLatency{ 5 };

    // Largest N. One decision per second of game.
    inline constexpr std::uint8_t k_MaxDecisionPeriod{ 60 };

    // Smallest d. With 1 the action of a tick could be waited for before the engine has read the tick it answers.
    // note: N is at least k_MinLatency and d at most N: each player has one action slot, so the client must not write decision t + N
    // while the game can still read decision t.
    inline constexpr std::uint8_t k_MinLatency{ 2 };

    // Owner, mode and agents in effect, decided by the bridge thread from the requests of the client (D106).
    struct BridgeControlState
    {
        bool IsClientConnected{ false };
        BridgeOwner Owner{ BridgeOwner::UI };
        BridgeMode Mode{ BridgeMode::Free };
        std::uint8_t DecisionPeriod{ k_DefaultDecisionPeriod };
        std::uint8_t Latency{ k_DefaultLatency };

        // Whether the client asks for the pipeline only on decision ticks (Q101). Not the effective one: see IsSparse.
        bool IsSparseRequested{ false };

        // Player table indices whose action comes from the client. Empty unless the client owns the instance.
        std::uint32_t AgentMask{ 0 };
    };

    // The wait of the game thread for a decision that starts between ticks, before the tick that needs it (TG.1, D380). Game thread only.
    struct BridgeHold
    {
        // Decision the game waits for, 0 = no wait started.
        std::uint64_t Decision{ 0 };

        // Action tick the tick gate predicted for the tick this wait is for. Only that tick continues the wait.
        std::uint64_t ActionTick{ 0 };

        // When the wait started, and when the time cap of the wait passes, as nanoseconds of the steady clock of the game thread (this project
        // keeps no clock: Bridge.Transport reads it and converts).
        std::int64_t StartNs{ 0 };
        std::int64_t DeadlineNs{ 0 };

        // QueryPerformanceCounter when the wait started: the real arrival at the decision.
        std::uint64_t StartQpc{ 0 };

        // Frames paused by the engine seen at the last check, to know when a pause went by.
        std::uint64_t PausedFrames{ 0 };

        // Whether a pause went by during the wait: pause time is not a cost.
        bool IsPauseSeen{ false };
    };

    // Work of the game thread between ticks (only the PlayerAction hook and the tick gate touch it).
    struct BridgeHookState
    {
        // The wait for the decision of the next tick that started between ticks (TG.1), continued by the PlayerAction hook.
        BridgeHold Hold{};

        // Decision whose actions were given to the players, 0 = none.
        std::uint64_t AppliedSequence{ 0 };

        // Players that have an AgentInput of the client, so they are released when they leave the mask.
        std::uint32_t ControlledMask{ 0 };

        // Lockstep timeouts in a row, and whether Lockstep is suspended until the client answers a decision again.
        std::uint32_t TimeoutsInRow{ 0 };
        bool IsLockstepSuspended{ false };

        // Last decision the game thread needed at its tick plus the latency (0 = none), when it arrived there and when the decision was
        // available (QueryPerformanceCounter: the same instant when no wait was needed), and how the wait went (BridgeNeedFlag bits). P.3 C.
        std::uint64_t NeedDecision{ 0 };
        std::uint64_t NeedQpc{ 0 };
        std::uint64_t NeedEndQpc{ 0 };
        std::uint32_t NeedFlags{ 0 };
    };

    // How a wait of the Lockstep for a decision ended.
    enum class WaitResult : std::uint8_t
    {
        // The client wrote the actions of the decision.
        Answered,
        // The wait stopped without an answer or a cost: the bridge was stopped, or the client left or was lost.
        Cut,
        // The time cap passed without the actions.
        TimedOut,
    };

    // What the tick gate does before a tick (TG.1, D380).
    enum class HoldAction : std::uint8_t
    {
        // The tick runs: the decision is there, or the tick has to go on and the PlayerAction hook settles it.
        Run,
        // The gate keeps waiting for the decision.
        Wait,
        // The frame ends here and the engine draws, the wait goes on in the next frame.
        Hold,
    };

    // Whether a decision is applied in this tick.
    enum class ApplyTiming : std::uint8_t
    {
        None,
        // The tick of the decision plus the latency.
        OnTime,
        // A later tick: the decision arrived after its tick.
        Late,
    };

    // What the clock of a read tick says about the one before.
    struct ClockReading
    {
        // First read of a match: every engine starts the clock again (D139).
        bool IsNewMatch{ false };

        // The action tick repeats: the engine applied no actions in the tick.
        bool IsRepeated{ false };

        // The action tick jumped in a run of consecutive reads.
        bool IsSkip{ false };
    };
}