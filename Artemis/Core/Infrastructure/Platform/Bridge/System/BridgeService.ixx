export module Platform.Bridge.System;

import Common.Log.System;
import Service.Settings.State;
import Control.Action.Type;
import Control.Lockstep.Type;
import Platform.Lifecycle.State;
import Platform.Input.State;
import Platform.Bridge.Type;
import Platform.Bridge.State;
import Export.Flat.Type;
import std;

export namespace Platform::Bridge::System
{
    // Bridge between Artemis and a Python client on the same PC (D135-D138): the shared memory, the control the client asks for, the
    // decision ticks and the actions of the players the client controls.
    // note: Three threads use it. The bridge thread opens and closes the region, runs Update and publishes the ticks. The AI thread
    // captures the clock while the game thread waits for the read of a tick. The game thread applies the actions in the PlayerAction
    // hook and, in Lockstep, waits there for them (D105).
    class BridgeService
    {
    private:
        using Log = Common::Log::System::Log;

        using SettingsStore = Service::Settings::State::SettingsStore;

        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using MatchCycleStore = Platform::Lifecycle::State::MatchCycleStore;

        using ActionOverride = Control::Action::Type::ActionOverride;

        using BridgeControlState = Control::Lockstep::Type::BridgeControlState;
        using BridgeHookState = Control::Lockstep::Type::BridgeHookState;

        using PlayerActionStore = Platform::Input::State::PlayerActionStore;

        using BridgeRegion = Platform::Bridge::Type::BridgeRegion;
        using BridgeTick = Platform::Bridge::Type::BridgeTick;
        using BridgeAction = Platform::Bridge::Type::BridgeAction;
        using BridgeClock = Export::Flat::Type::BridgeClock;
        using BridgeLinkState = Platform::Bridge::Type::BridgeLinkState;
        using BridgeStatus = Platform::Bridge::Type::BridgeStatus;

        using BridgeStore = Platform::Bridge::State::BridgeStore;

    public:
        BridgeService(Log& log, SettingsStore& settingsStore, LifecycleStore& lifecycleStore,
            MatchCycleStore& matchCycleStore, PlayerActionStore& playerActionStore, BridgeStore& bridgeStore) :
            m_Log(log), m_SettingsStore(settingsStore), m_LifecycleStore(lifecycleStore),
            m_MatchCycleStore(matchCycleStore), m_PlayerActionStore(playerActionStore), m_BridgeStore(bridgeStore) {}
        ~BridgeService() = default;

        BridgeService(const BridgeService&) = delete;
        auto operator=(const BridgeService&) -> BridgeService& = delete;

        // Creates the region "Local\Artemis.Bridge.<pid>", its two events and the discovery file (D136).
        // return: False if the region could not be created (the bridge stays off, the game is not affected).
        // note: Bridge thread.
        auto Open() -> bool;

        // Tells the client the bridge is gone (Magic 0, discovery file removed) and stops the waits of the game thread.
        // note: The view stays mapped until the process ends, because a hook in flight may still read it.
        auto Close() -> void;

        // Stops the Lockstep waits of the game thread until the next BeginMatch (unhook, shutdown).
        auto Stop() -> void;

        // Starts the state of a new engine: the action tick of the PlayerAction hook starts over, so the decisions, the players and
        // the overrides of the previous match are forgotten, and the waits are allowed again.
        // note: Game thread (engine initialization), before the PlayerAction hook is installed.
        auto BeginMatch() -> void;

        // Heartbeat, client connection, owner, mode, scale, pause and counters of the control block. Bridge thread, every k_ControlInterval.
        // note: When the client leaves or is lost its players are released on the next tick: locals go back to their input, bots stand
        // still.
        auto Update() -> void;

        // Reads the clock of the tick being read. AI thread, while the game thread waits for the read (SyncRead).
        // param isConsecutive: False after dropped ticks or a reset, so a jump is not counted as a clock skip.
        // note: The first read of a new match is not a skip either: every engine starts the clock again (D139).
        auto CaptureClock(bool isConsecutive, BridgeClock& out) -> void;

        // Forgets the last read tick (the AI thread starts over after a teardown).
        auto ResetClock() -> void;

        // Records that the AI thread handed this decision tick to the bridge thread. AI thread.
        auto MarkPushed(std::uint64_t tick) -> void;

        // return: The buffer the next tick goes to, marked as being written, or null when the bridge is closed.
        // note: Bridge thread. Every BeginTick is followed by EndTick.
        auto BeginTick() -> BridgeTick*;

        // Completes the BridgeTick of BeginTick with the clock, publishes it and wakes the client.
        auto EndTick(BridgeTick& flat, const BridgeClock& clock) -> void;

        // Applies the actions of the client to its players before the engine applies the actions of a tick, waiting for them in
        // Lockstep. Game thread (PlayerAction hook), once per tick.
        // param actionTick: Number of the action tick about to be applied (1 + the ticks applied before).
        auto Apply(std::uint64_t actionTick) -> void;

        auto CopyStatus(BridgeStatus& out) const -> void;

    private:
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        LifecycleStore& m_LifecycleStore;
        MatchCycleStore& m_MatchCycleStore;
        PlayerActionStore& m_PlayerActionStore;
        BridgeStore& m_BridgeStore;

        auto WriteDiscovery(std::uint32_t pid, const std::string& name) -> std::string;
        auto LogTransition(const BridgeControlState& previous, const BridgeControlState& control, bool isClientLeft) -> void;
        auto WriteArtemisControl(BridgeRegion& region, const BridgeControlState& control) -> void;
        auto ApplyPause(bool isRequested) -> void;

        auto WaitForDecision(BridgeRegion& region, std::uint64_t decision, BridgeHookState& hook) -> std::uint64_t;
        auto ApplyDecision(const BridgeRegion& region, std::uint64_t decision, std::uint32_t agentMask, BridgeHookState& hook) -> void;
        auto ReleasePlayers(BridgeHookState& hook, std::uint32_t keepMask) -> void;
        auto ReleasePlayer(BridgeHookState& hook, std::size_t player) -> void;
    };
}