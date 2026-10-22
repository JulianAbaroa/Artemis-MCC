export module Platform.Bridge.System;

import Service.Logs.System;
import Service.Settings.State;
import Platform.Lifecycle.State;
import Platform.Input.Type;
import Platform.Input.State;
import Platform.Bridge.Type;
import Platform.Bridge.State;
import std;

export namespace Platform::Bridge::System
{
    // Bridge between Artemis and a Python client on the same PC (D135-D138): the shared memory, the control the client asks for, the
    // frames of the decision ticks and the actions of the players the client controls.
    // note: Three threads use it. The bridge thread opens and closes the region, runs Update and publishes the frames. The AI thread
    // captures the clock while the game thread waits for the read of a tick. The game thread applies the actions in the PlayerAction
    // hook and, in Lockstep, waits there for them (D105).
    class BridgeService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using SettingsStore = Service::Settings::State::SettingsStore;

        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using MatchCycleStore = Platform::Lifecycle::State::MatchCycleStore;

        using ActionOverride = Platform::Input::Type::ActionOverride;

        using PlayerActionStore = Platform::Input::State::PlayerActionStore;

        using BridgeRegion = Platform::Bridge::Type::BridgeRegion;
        using BridgeFrame = Platform::Bridge::Type::BridgeFrame;
        using BridgeAction = Platform::Bridge::Type::BridgeAction;
        using BridgeClock = Platform::Bridge::Type::BridgeClock;
        using BridgeControlState = Platform::Bridge::Type::BridgeControlState;
        using BridgeHookState = Platform::Bridge::Type::BridgeHookState;
        using BridgeLinkState = Platform::Bridge::Type::BridgeLinkState;
        using BridgeStatus = Platform::Bridge::Type::BridgeStatus;

        using BridgeStore = Platform::Bridge::State::BridgeStore;

    public:
        BridgeService(LogsService& logsService, SettingsStore& settingsStore, LifecycleStore& lifecycleStore,
            MatchCycleStore& matchCycleStore, PlayerActionStore& playerActionStore, BridgeStore& bridgeStore) :
            m_LogsService(logsService), m_SettingsStore(settingsStore), m_LifecycleStore(lifecycleStore),
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

        // Records that the AI thread handed the frame of this decision tick to the bridge thread. AI thread.
        auto MarkPushed(std::uint64_t tick) -> void;

        // return: The buffer the next frame goes to, marked as being written, or null when the bridge is closed.
        // note: Bridge thread. Every BeginFrame is followed by EndFrame.
        auto BeginFrame() -> BridgeFrame*;

        // Completes the frame of BeginFrame with the clock, publishes it and wakes the client.
        auto EndFrame(BridgeFrame& frame, const BridgeClock& clock) -> void;

        // Applies the actions of the client to its players before the engine applies the actions of a tick, waiting for them in
        // Lockstep. Game thread (PlayerAction hook), once per tick.
        // param actionTick: Number of the action tick about to be applied (1 + the ticks applied before).
        auto Apply(std::uint64_t actionTick) -> void;

        auto CopyStatus(BridgeStatus& out) const -> void;

    private:
        LogsService& m_LogsService;
        SettingsStore& m_SettingsStore;
        LifecycleStore& m_LifecycleStore;
        MatchCycleStore& m_MatchCycleStore;
        PlayerActionStore& m_PlayerActionStore;
        BridgeStore& m_BridgeStore;

        auto WriteDiscovery(std::uint32_t pid, const std::string& name) -> std::string;
        auto DecideControl(const BridgeRegion& region, const BridgeControlState& previous) -> BridgeControlState;
        auto LogTransition(const BridgeControlState& previous, const BridgeControlState& control, bool isClientLeft) -> void;
        auto WriteArtemisControl(BridgeRegion& region, const BridgeControlState& control) -> void;
        auto ApplyPause(bool isRequested) -> void;

        auto WaitForDecision(BridgeRegion& region, std::uint64_t decision, BridgeHookState& hook) -> std::uint64_t;
        auto ApplyDecision(const BridgeRegion& region, std::uint64_t decision, std::uint32_t agentMask, BridgeHookState& hook) -> void;
        auto BuildOverride(const BridgeAction& action) -> ActionOverride;
        auto ReleasePlayers(BridgeHookState& hook, std::uint32_t keepMask) -> void;
        auto ReleasePlayer(BridgeHookState& hook, std::size_t player) -> void;
    };
}