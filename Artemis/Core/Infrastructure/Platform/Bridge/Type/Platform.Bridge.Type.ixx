export module Platform.Bridge.Type;

export import :Layout;

import Common.Engine.Type;
import Platform.Input.Type;
import std;

namespace
{
    using Platform::Input::Type::ActionOverride;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;
}

export namespace Platform::Bridge::Type
{
    // Time without a new client heartbeat after which Python counts as gone: its players go back to the engine and the UI owns
    // the instance again (D106).
    inline constexpr std::chrono::milliseconds k_ClientTimeout{ 2000 };

    // Longest wait of the game thread for the actions of one decision in Lockstep. After it the previous actions are kept (D105).
    // note: Below the read timeout of the tick, so a stalled client never looks like a stalled AI thread.
    inline constexpr std::chrono::milliseconds k_LockstepTimeout{ 1000 };

    // Slice of a Lockstep wait: how often the wait checks that the bridge was not stopped (the hook drain is 500 ms).
    inline constexpr std::chrono::milliseconds k_LockstepWaitSlice{ 10 };

    // Lockstep timeouts in a row after which Lockstep is suspended until the client answers again.
    inline constexpr std::uint32_t k_MaxLockstepTimeouts{ 2 };

    // Time between two updates of the control block (heartbeat, owner, mode, counters).
    inline constexpr std::chrono::milliseconds k_ControlInterval{ 50 };

    // Decision period N and latency d until the client asks for others (D114).
    inline constexpr std::uint8_t k_DefaultDecisionPeriod{ 4 };
    inline constexpr std::uint8_t k_DefaultLatency{ 2 };

    // Largest N. One decision per second of game.
    inline constexpr std::uint8_t k_MaxDecisionPeriod{ 60 };

    // Smallest d. With 1 the action of a tick could be waited for before the engine has read the tick it answers.
    // note: N is at least k_MinLatency and d at most N: each player has one action slot, so the client must not write decision t + N
    // while the game can still read decision t.
    inline constexpr std::uint8_t k_MinLatency{ 2 };

    // Action tick and applied decisions read while the game thread waits for the read of a tick, so they match the state of the
    // tables copied in that tick.
    struct BridgeClock
    {
        // Last action tick the engine applied before this state.
        std::uint64_t ActionTick{ 0 };

        // DecisionTick of the bridge action applied to each player (by player table index), 0 = none.
        std::array<std::uint64_t, k_MaxPlayers> AppliedDecisions{};

        // Whether this state is a decision tick of a connected client (a frame goes out).
        bool IsDecision{ false };
    };

    // Owner, mode and agents in effect, decided by the bridge thread from the requests of the client (D106).
    struct BridgeControlState
    {
        bool IsClientConnected{ false };
        BridgeOwner Owner{ BridgeOwner::UI };
        BridgeMode Mode{ BridgeMode::Free };
        std::uint8_t DecisionPeriod{ k_DefaultDecisionPeriod };
        std::uint8_t Latency{ k_DefaultLatency };

        // Player table indices whose action comes from the client. Empty unless the client owns the instance.
        std::uint32_t AgentMask{ 0 };
    };

    // Player of each slot in the last published frame: the actions of a slot must carry its handle (D40).
    struct BridgeSlot
    {
        std::uint32_t Handle{ k_NoHandle };

        // Split-screen index, so the view of a local player follows its action. Empty for bots.
        std::optional<std::size_t> LocalIndex{};
    };

    // Work of the game thread between ticks (only the PlayerAction hook touches it).
    struct BridgeHookState
    {
        // Decision whose actions are in the overrides, 0 = none.
        std::uint64_t AppliedSequence{ 0 };

        // Players that have a bridge override, so they are cleared when they leave the mask.
        std::uint32_t ControlledMask{ 0 };

        // Lockstep timeouts in a row, and whether Lockstep is suspended until the client answers a decision again.
        std::uint32_t TimeoutsInRow{ 0 };
        bool IsLockstepSuspended{ false };

        // Override of each player of the client, set again every tick so it wins over the UI and the bench (D106).
        std::array<std::optional<ActionOverride>, k_MaxPlayers> Overrides{};
    };

    // Link with the client as the bridge thread follows it (only the bridge thread touches it).
    struct BridgeLinkState
    {
        // Counter raised for the client every update.
        std::uint64_t Heartbeat{ 0 };

        // Last client heartbeat seen and when it changed.
        std::uint64_t LastClientHeartbeat{ 0 };
        std::chrono::steady_clock::time_point LastClientBeatTime{};

        // Requested time scale and pause of the UI when the client took the instance, given back when it leaves.
        float SavedTimeScale{ 0.0f };
        bool IsSavedPauseRequested{ false };

        // Last RequestEndRound of the client seen. A change while Python owns the instance is a request (D144).
        std::uint32_t LastClientEndRound{ 0 };

        // When the frame being written started, to time the flatten.
        std::chrono::steady_clock::time_point FrameStart{};
    };

    // Counters of the bridge, mirrored into the control block and shown in the UI.
    struct BridgeCounters
    {
        std::uint64_t Frames{ 0 };
        std::uint64_t LateActions{ 0 };
        std::uint64_t LockstepWaits{ 0 };
        std::uint64_t LockstepWaitNs{ 0 };
        std::uint64_t LockstepWaitMaxNs{ 0 };
        std::uint64_t LockstepTimeouts{ 0 };
        std::uint64_t HandleMismatches{ 0 };
        std::uint64_t RejectedActions{ 0 };
        std::uint64_t ClockSkips{ 0 };
        std::uint64_t FlattenNs{ 0 };
        std::uint64_t FlattenMaxNs{ 0 };
    };

    // State of the bridge as the UI shows it.
    struct BridgeStatus
    {
        bool IsOpen{ false };
        bool IsClientConnected{ false };
        BridgeOwner Owner{ BridgeOwner::UI };
        BridgeMode Mode{ BridgeMode::Free };
        std::uint8_t DecisionPeriod{ k_DefaultDecisionPeriod };
        std::uint8_t Latency{ k_DefaultLatency };
        std::uint32_t AgentMask{ 0 };
        std::uint32_t Pid{ 0 };
        BridgeCounters Counters{};
    };
}