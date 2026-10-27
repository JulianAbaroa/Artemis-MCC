export module UI.Simulation.System;

import Common.Log.System;
import Service.Telemetry.State;
import Control.Action.Type;
import Platform.Lifecycle.State;
import Platform.Match.State;
import Platform.Match.System;
import Platform.Host.State;
import Platform.Input.Type;
import Platform.Input.State;
import Platform.Simulation.State;
import Platform.Bridge.System;
import Environment.Players.Type;
import Export.Tick.Type;
import Export.Tick.State;
import UI.Simulation.Type;
import UI.Simulation.State;
import std;

namespace
{
    using Platform::Input::Type::PlayerAction;
    using Control::Action::Type::ActionOverride;
    using UI::Simulation::Type::LocalPlayerAction;
}

export namespace UI::Simulation::System
{
    // Builds the override that plays a test mode for one player, keeping the facing of its last action (D109, D119). Used by the
    // simulation tab and by the test bench (D129).
    // param localPlayer: Local index for a local player, empty for a bot.
    // return: Empty for LocalPlayerAction::Engine: the action the engine built plays.
    auto BuildActionOverride(LocalPlayerAction mode, const PlayerAction& action, std::optional<std::size_t> localPlayer)
        -> std::optional<ActionOverride>;

    // Draws the simulation tab (D107): the match config of the next start (D123), game speed, local players (split-screen) with their
    // input, the player actions of each tick, the machines and players of the engine, and the bots (A9 M5).
    class SimulationUIService
    {
    private:
        using Log = Common::Log::System::Log;
        using TelemetryStore = Service::Telemetry::State::TelemetryStore;

        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using MatchConfigStore = Platform::Match::State::MatchConfigStore;
        using MatchConfigService = Platform::Match::System::MatchConfigService;
        using LocalUserStore = Platform::Host::State::LocalUserStore;

        using InputState = Platform::Input::Type::InputState;
        using PlayerAction = Platform::Input::Type::PlayerAction;
        using PlayerActions = Platform::Input::Type::PlayerActions;
        using ActionWatch = Platform::Input::Type::ActionWatch;
        using VirtualInputStore = Platform::Input::State::VirtualInputStore;
        using PlayerActionStore = Platform::Input::State::PlayerActionStore;

        using BotStore = Platform::Simulation::State::BotStore;

        using BridgeService = Platform::Bridge::System::BridgeService;

        using Players = Environment::Players::Type::Players;
        using TickStore = Export::Tick::State::TickStore;

        using LocalPlayerInput = UI::Simulation::Type::LocalPlayerInput;
        using LocalPlayerAction = UI::Simulation::Type::LocalPlayerAction;
        using SimulationUIStore = UI::Simulation::State::SimulationUIStore;

    public:
        SimulationUIService(Log& log, TelemetryStore& telemetryStore, LifecycleStore& lifecycleStore,
            MatchConfigStore& matchConfigStore, MatchConfigService& matchConfigService, LocalUserStore& localUserStore,
            VirtualInputStore& virtualInputStore, PlayerActionStore& playerActionStore, BotStore& botStore,
            BridgeService& bridgeService, TickStore& tickStore, SimulationUIStore& simulationUIStore) :
            m_Log(log), m_TelemetryStore(telemetryStore), m_LifecycleStore(lifecycleStore),
            m_MatchConfigStore(matchConfigStore), m_MatchConfigService(matchConfigService), m_LocalUserStore(localUserStore),
            m_VirtualInputStore(virtualInputStore), m_PlayerActionStore(playerActionStore),
            m_BotStore(botStore), m_BridgeService(bridgeService), m_TickStore(tickStore), m_SimulationUIStore(simulationUIStore) {}
        ~SimulationUIService() = default;

        SimulationUIService(const SimulationUIService&) = delete;
        auto operator=(const SimulationUIService&) -> SimulationUIService& = delete;

        // Draws the content of the simulation window.
        auto Draw() -> void;

    private:
        Log& m_Log;
        TelemetryStore& m_TelemetryStore;
        LifecycleStore& m_LifecycleStore;
        MatchConfigStore& m_MatchConfigStore;
        MatchConfigService& m_MatchConfigService;
        LocalUserStore& m_LocalUserStore;
        VirtualInputStore& m_VirtualInputStore;
        PlayerActionStore& m_PlayerActionStore;
        BotStore& m_BotStore;
        BridgeService& m_BridgeService;
        TickStore& m_TickStore;
        SimulationUIStore& m_SimulationUIStore;

        // Draws the record of the next start, the library of recorded configurations and the one the next start transplants (D123).
        auto DrawMatchConfig() -> void;

        // Draws the requested and the real engine speed, the presets and the custom field.
        auto DrawGameSpeed() -> void;

        // Draws the state of the bridge with Python (D135): client, owner, mode and counters. Read only.
        auto DrawBridge() -> void;

        // Draws the local player count and one row per player.
        auto DrawLocalPlayers() -> void;

        // Draws the name, the engine status, the engine player and biped, and the input combo of a player.
        // param players: Players of the latest tick, or null before the first one.
        auto DrawLocalPlayerRow(std::size_t player, std::size_t requestedCount, const Players* players) -> void;

        // Applies an input choice to the virtual input of a player.
        auto ApplyLocalPlayerInput(std::size_t player, LocalPlayerInput input) -> void;

        // Draws the action of one local player in the latest tick (buttons, facing, throttle, bytes) and its action mode (A9 M4).
        auto DrawPlayerActions() -> void;

        // Draws the 64 button bits: held this tick, held at some tick since the last clear, or never.
        static auto DrawButtonBits(std::uint64_t buttons, std::uint64_t seen) -> void;

        // Draws the bytes of the action, 16 per row, the ones that changed at some tick since the last clear highlighted.
        static auto DrawActionBytes(const PlayerAction& action, const ActionWatch& watch) -> void;

        // Writes the override of the action mode of a local player, by its player table index.
        // param action: Latest action of the player. Hold still and Test action start from its facing.
        auto ApplyLocalPlayerAction(std::size_t localPlayer, std::size_t playerIndex, LocalPlayerAction mode,
            const PlayerAction& action) -> void;

        // Writes the action and what changed since the last clear to the log, so a measurement can be shared.
        auto LogAction(std::size_t localPlayer, std::uint64_t sequence, const PlayerAction& action, const ActionWatch& watch) -> void;

        // Draws the machines and players of the engine and the game options entry of one player (A9 M5a).
        auto DrawSession() -> void;

        // Writes the game options entry of every valid player to the log, so the configuration layout can be shared.
        auto LogOptions() -> void;

        // Draws the add button and one row per bot with its action mode (A9 M5b).
        auto DrawBots() -> void;

        // Draws the shadow check (D122): its switch and the comparisons of every field.
        auto DrawShadow() -> void;

        // Writes the latest action of a bot to the log (bytes and whether it is in the mask).
        auto LogBotAction(std::size_t number, std::size_t playerIndex, const PlayerActions& actions) -> void;

        // Writes the override of the action mode of a bot, by its player table index. Hold still and Test action start from the
        // facing of its latest action.
        auto ApplyBotAction(std::size_t playerIndex, LocalPlayerAction mode) -> void;

        // return: The virtual state of a test input. Empty for Idle.
        static auto BuildTestState(LocalPlayerInput input) -> InputState;
    };
}