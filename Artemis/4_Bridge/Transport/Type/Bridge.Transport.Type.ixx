export module Bridge.Transport.Type;

export import Contract.Bridge.Type;

import Common.Engine.Type;
import Control.Lockstep.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Contract::Bridge::Type::BridgeRoster;
    using Contract::Bridge::Type::BridgeSetupResult;
    using Control::Lockstep::Type::BridgeControlState;
}

export namespace Bridge::Transport::Type
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

    // One match setup the client asks for (LS.2): the fields it did not set are empty and mean no change. An empty preset asks for the base
    // values and an empty map for the map variant the match loaded with.
    struct BridgeSetupRequest
    {
        std::optional<std::string> Preset{};
        std::optional<std::string> Map{};
        std::optional<BridgeRoster> Roster{};
    };

    // What became of the last setup request: the result, the round of the match it was applied in (-1 when none) and why, in ASCII.
    struct BridgeSetupInfo
    {
        BridgeSetupResult Result{ BridgeSetupResult::NoResult };
        std::int16_t Round{ -1 };
        std::string Reason{};
    };

    // The setup the engine holds: the names of the preset and of the map variant, empty for none, and the roster if it has one.
    struct BridgeActiveSetup
    {
        std::string Preset{};
        std::string Map{};
        std::optional<BridgeRoster> Roster{};
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

        // Last RequestSetup of the client seen. A change while Python owns the instance is a request for the fields of RequestSetupFields
        // (LS.2): the preset, the map variant and the roster of one match setup.
        std::uint32_t LastClientSetup{ 0 };

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

        // Client link, owner, mode and agents in effect.
        BridgeControlState Control{};
        std::uint32_t Pid{ 0 };
        BridgeCounters Counters{};
    };
}