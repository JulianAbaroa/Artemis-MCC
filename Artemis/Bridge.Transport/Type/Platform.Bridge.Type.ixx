export module Platform.Bridge.Type;

export import Contract.Bridge;

import Common.Engine.Type;
import Control.Lockstep.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Control::Lockstep::Type::k_DefaultDecisionPeriod;
    using Control::Lockstep::Type::k_DefaultLatency;
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

    // Time between two updates of the control block (heartbeat, owner, mode, counters).
    inline constexpr std::chrono::milliseconds k_ControlInterval{ 50 };

    // Player of each slot in the last published tick: the actions of a slot must carry its handle (D40).
    struct BridgeSlot
    {
        std::uint32_t Handle{ k_NoHandle };

        // Split-screen index, so the view of a local player follows its action. Empty for bots.
        std::optional<std::size_t> LocalIndex{};
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

        // When the tick being written started, to time the flatten.
        std::chrono::steady_clock::time_point PublishStart{};
    };

    // Counters of the bridge, mirrored into the control block and shown in the UI.
    struct BridgeCounters
    {
        std::uint64_t PublishedTicks{ 0 };
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