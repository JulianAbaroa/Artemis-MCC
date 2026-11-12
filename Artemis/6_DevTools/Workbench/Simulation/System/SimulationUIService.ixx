export module DevTools.Workbench.Simulation.System;

import Common.Log.System;
import Services.Telemetry.State;
import Control.Action.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.MatchSetup.System;
import Engine.Match.State;
import Engine.Match.System;
import Engine.Base.Host.State;
import Engine.Control.Input.Type;
import Engine.Control.Input.State;
import Engine.Control.Simulation.Type;
import Engine.Control.Simulation.State;
import Bridge.Transport.System;
import Perception.Players.Type;
import Bridge.Codec.Tick.Type;
import Bridge.Codec.Tick.State;
import DevTools.Workbench.Simulation.Type;
import DevTools.Workbench.Simulation.State;
import DevTools.Workbench.TickDump.State;
import DevTools.Workbench.Bench.State;
import std;

namespace
{
    using Engine::Control::Input::Type::PlayerAction;
    using Control::Action::Type::ActionOverride;
    using DevTools::Workbench::Simulation::Type::LocalPlayerAction;
}

export namespace DevTools::Workbench::Simulation::System
{
    // Builds the override that plays a test mode for one player, keeping the facing of its last action (D109, D119). Used by the
    // simulation tab and by the test bench (D129).
    // param localPlayer: Local index for a local player, empty for a bot.
    // return: Empty for LocalPlayerAction::Engine: the action the engine built plays.
    auto BuildActionOverride(LocalPlayerAction mode, const PlayerAction& action, std::optional<std::size_t> localPlayer)
        -> std::optional<ActionOverride>;

    // Draws the simulation tab (D107, D289): a fixed strip with the game speed and the state of the bridge, the dump, the bench and the
    // bots, then the collapsible sections MATCH, BOTS, TICK DUMP, LOCAL PLAYERS and DEVELOPMENT. A closed section copies and walks
    // nothing. Every control carries a tooltip, and one that cannot act is disabled with the reason.
    class SimulationUIService
    {
    private:
        using Log = Common::Log::System::Log;
        using TelemetryStore = Services::Telemetry::State::TelemetryStore;

        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using MatchSetupService = Engine::Base::MatchSetup::System::MatchSetupService;
        using MatchConfigStore = Engine::Match::State::MatchConfigStore;
        using MatchCycleStore = Engine::Match::State::MatchCycleStore;
        using MatchConfigService = Engine::Match::System::MatchConfigService;
        using LocalUserStore = Engine::Base::Host::State::LocalUserStore;

        using InputState = Engine::Control::Input::Type::InputState;
        using PlayerAction = Engine::Control::Input::Type::PlayerAction;
        using PlayerActions = Engine::Control::Input::Type::PlayerActions;
        using ActionWatch = Engine::Control::Input::Type::ActionWatch;
        using VirtualInputStore = Engine::Control::Input::State::VirtualInputStore;
        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;

        using BotStore = Engine::Control::Simulation::State::BotStore;
        using Bot = Engine::Control::Simulation::Type::Bot;
        using SessionPlayer = Engine::Control::Simulation::Type::SessionPlayer;
        using SessionSnapshot = Engine::Control::Simulation::Type::SessionSnapshot;

        using BridgeService = Bridge::Transport::System::BridgeService;

        using Players = Perception::Players::Type::Players;
        using TickStore = Bridge::Codec::Tick::State::TickStore;

        using LocalPlayerInput = DevTools::Workbench::Simulation::Type::LocalPlayerInput;
        using LocalPlayerAction = DevTools::Workbench::Simulation::Type::LocalPlayerAction;
        using SimulationUIStore = DevTools::Workbench::Simulation::State::SimulationUIStore;
        using SimulationSection = DevTools::Workbench::Simulation::Type::SimulationSection;
        using TickDumpUIStore = DevTools::Workbench::TickDump::State::TickDumpUIStore;
        using BenchUIStore = DevTools::Workbench::Bench::State::BenchUIStore;

    public:
        SimulationUIService(Log& log, TelemetryStore& telemetryStore, LifecycleStore& lifecycleStore,
            MatchConfigStore& matchConfigStore, MatchConfigService& matchConfigService, LocalUserStore& localUserStore,
            VirtualInputStore& virtualInputStore, PlayerActionStore& playerActionStore, BotStore& botStore,
            MatchSetupService& matchSetupService, MatchCycleStore& matchCycleStore, BridgeService& bridgeService, TickStore& tickStore, SimulationUIStore& simulationUIStore,
            TickDumpUIStore& tickDumpUIStore, BenchUIStore& benchUIStore) :
            m_Log(log), m_TelemetryStore(telemetryStore), m_LifecycleStore(lifecycleStore),
            m_MatchConfigStore(matchConfigStore), m_MatchConfigService(matchConfigService), m_LocalUserStore(localUserStore),
            m_VirtualInputStore(virtualInputStore), m_PlayerActionStore(playerActionStore),
            m_BotStore(botStore), m_MatchSetupService(matchSetupService), m_MatchCycleStore(matchCycleStore), m_BridgeService(bridgeService), m_TickStore(tickStore), m_SimulationUIStore(simulationUIStore),
            m_TickDumpUIStore(tickDumpUIStore), m_BenchUIStore(benchUIStore) {}
        ~SimulationUIService() = default;

        SimulationUIService(const SimulationUIService&) = delete;
        auto operator=(const SimulationUIService&) -> SimulationUIService& = delete;

        // Draws the content of the simulation window.
        // param drawTickDump: Draws the body of the TICK DUMP section. It lives in another folder that imports this one, so the layer passes it.
        // param drawDevelopmentTools: Draws the test bench and the raycast oracle inside DEVELOPMENT, for the same reason.
        auto Draw(const std::function<void()>& drawTickDump, const std::function<void()>& drawDevelopmentTools) -> void;

        // Draws the windows opened from the tab (player actions and shadow check). Costs nothing while they are closed.
        auto DrawWindows() -> void;

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
        MatchSetupService& m_MatchSetupService;
        MatchCycleStore& m_MatchCycleStore;
        BridgeService& m_BridgeService;
        TickStore& m_TickStore;
        SimulationUIStore& m_SimulationUIStore;
        TickDumpUIStore& m_TickDumpUIStore;
        BenchUIStore& m_BenchUIStore;

        // Draws the state of the match, the library of recorded configurations with its actions, the pending one and the record (D123).
        auto DrawMatchConfig() -> void;

        // Draws the chip with the state of the match: lobby, loading, or in match with its configuration and round.
        auto DrawMatchState() -> void;

        // Draws the rename and delete buttons of the picked configuration and their popups.
        // param name: The picked configuration, empty if none.
        auto DrawConfigEdit(const std::string& name) -> void;

        // Lists the folder again and keeps the pick when its name is still there.
        // param prefer: Name to pick when it is there, instead of the one picked before.
        auto RefreshConfigs(std::string_view prefer = {}) -> void;

        // Draws the header of a section and remembers whether the user folded it.
        // return: Whether the body must be drawn.
        auto BeginSection(SimulationSection section) -> bool;

        // Draws the fixed strip: speed controls and the state dots.
        auto DrawStrip() -> void;

        // Draws pause, speed presets, the custom field and the real speed, wrapping to the next line when a control does not fit.
        auto DrawSpeedRow() -> void;

        // Draws the dots of the bridge, the dump and the bench, and the bot count.
        auto DrawStatusRow() -> void;

        // Draws the body of DEVELOPMENT: bridge, bench, oracle, session and the button of the player actions window.
        auto DrawDevelopment(const std::function<void()>& drawDevelopmentTools) -> void;

        // Draws the state of the bridge with Python: client, mode and owner in a line, the counters in its tooltip.
        auto DrawBridge() -> void;

        // Draws the count of the next start and one row per player.
        auto DrawLocalPlayers() -> void;

        // Draws the name, the state and the input combo of a player. The engine handle and biped are in the tooltip of the row.
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

        // Draws the machines of the engine, its players and the detail of the picked one.
        auto DrawSession() -> void;

        // Writes the game options entry of every valid player to the log, so the configuration layout can be shared.
        auto LogOptions() -> void;

        // Draws the roster editor with its buttons and the chip of the last request, the table of the bots and the detail of the picked one.
        auto DrawBots() -> void;

        // Draws the counters of the roster editor (Red, Blue, Auto and, folded, the other six teams), the Apply now and Next round buttons
        // and the chip with the result of the last request.
        auto DrawRoster() -> void;

        // Draws the table of the bots: number, team, state and mode.
        auto DrawBotTable(const std::vector<Bot>& bots, const SessionSnapshot& snapshot) -> void;

        // Draws the detail of one bot: its ids, the team, the action mode, the one-tick requests and Remove.
        auto DrawBotDetail(const Bot& bot, const SessionPlayer& player) -> void;

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