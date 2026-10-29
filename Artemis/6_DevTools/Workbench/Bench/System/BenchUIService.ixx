export module DevTools.Workbench.Bench.System;

import Common.Log.System;
import Services.Logs.State;
import Services.Settings.State;
import Engine.Base.Memory.System;
import Engine.Base.Host.State;
import Engine.Control.Input.State;
import Engine.Base.Lifecycle.State;
import Engine.Match.State;
import Engine.Match.System;
import Engine.Control.Simulation.State;
import Bridge.Codec.Tick.State;
import DevTools.Workbench.TickTrace.State;
import DevTools.Workbench.Bench.Type;
import DevTools.Workbench.Bench.State;
import std;

export namespace DevTools::Workbench::Bench::System
{
    // Runs the scenarios of the in-game test bench (D129, D130): takes them from the inbox of the exchange folder, plays their matches
    // one after the other through the match cycle of MCC (A15 H3), checks each match and writes state.json and the report that
    // Tools/game.py reads. A match that fails or passes its limit is left with Leave and the next one starts (D131).
    // note: Development tool (D72). Update runs on the bench thread and Draw on the render thread, which only touches the store and
    // the files of the exchange folder. It never calls engine or MCC functions: the launch, the Leave, the
    // bots, the local users and the speed are requests the hooks apply on their own threads.
    class BenchUIService
    {
    private:
        using path = std::filesystem::path;

        using Log = Common::Log::System::Log;
        using LogsStore = Services::Logs::State::LogsStore;
        using SettingsStore = Services::Settings::State::SettingsStore;

        using MemoryReaderService = Engine::Base::Memory::System::MemoryReaderService;
        using LocalUserStore = Engine::Base::Host::State::LocalUserStore;
        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;
        using VirtualInputStore = Engine::Control::Input::State::VirtualInputStore;
        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using MatchConfigStore = Engine::Match::State::MatchConfigStore;
        using MatchCycleStore = Engine::Match::State::MatchCycleStore;
        using MatchConfigService = Engine::Match::System::MatchConfigService;
        using BotStore = Engine::Control::Simulation::State::BotStore;

        using TickStore = Bridge::Codec::Tick::State::TickStore;

        using TickTraceUIStore = DevTools::Workbench::TickTrace::State::TickTraceUIStore;

        using BenchProbe = DevTools::Workbench::Bench::Type::BenchProbe;
        using BenchScenario = DevTools::Workbench::Bench::Type::BenchScenario;
        using BenchMatchReport = DevTools::Workbench::Bench::Type::BenchMatchReport;
        using BenchRun = DevTools::Workbench::Bench::Type::BenchRun;
        using BenchUIStore = DevTools::Workbench::Bench::State::BenchUIStore;

    public:
        BenchUIService(LogsStore& logsStore, Log& log, SettingsStore& settingsStore,
            MemoryReaderService& memoryReaderService, LocalUserStore& localUserStore, PlayerActionStore& playerActionStore,
            VirtualInputStore& virtualInputStore,
            LifecycleStore& lifecycleStore, MatchConfigStore& matchConfigStore, MatchCycleStore& matchCycleStore,
            MatchConfigService& matchConfigService, BotStore& botStore, TickStore& tickStore, TickTraceUIStore& tickTraceUIStore,
            BenchUIStore& benchUIStore) :
            m_LogsStore(logsStore), m_Log(log), m_SettingsStore(settingsStore),
            m_MemoryReaderService(memoryReaderService), m_LocalUserStore(localUserStore), m_PlayerActionStore(playerActionStore),
            m_VirtualInputStore(virtualInputStore),
            m_LifecycleStore(lifecycleStore), m_MatchConfigStore(matchConfigStore), m_MatchCycleStore(matchCycleStore),
            m_MatchConfigService(matchConfigService), m_BotStore(botStore), m_TickStore(tickStore), m_TickTraceUIStore(tickTraceUIStore),
            m_BenchUIStore(benchUIStore) {}
        ~BenchUIService() = default;

        BenchUIService(const BenchUIService&) = delete;
        auto operator=(const BenchUIService&) -> BenchUIService& = delete;

        // Turns the bench on when the bench file is next to the DLL (the game watcher writes it on deploy) and creates the folders of
        // the exchange. Without the file the bench stays off and Update does nothing.
        auto Initialize() -> void;

        // Advances the running scenario, or takes the next one from the inbox, and writes state.json every second.
        auto Update() -> void;

        // Draws the bench panel of the Simulation tab: what the bench is doing, and a scenario of scenarios/ to queue, so a rotation
        // runs without Tools/game.py (Tools/game.py export writes those files).
        auto Draw() -> void;

    private:
        LogsStore& m_LogsStore;
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        MemoryReaderService& m_MemoryReaderService;
        LocalUserStore& m_LocalUserStore;
        PlayerActionStore& m_PlayerActionStore;
        VirtualInputStore& m_VirtualInputStore;
        LifecycleStore& m_LifecycleStore;
        MatchConfigStore& m_MatchConfigStore;
        MatchCycleStore& m_MatchCycleStore;
        MatchConfigService& m_MatchConfigService;
        BotStore& m_BotStore;
        TickStore& m_TickStore;
        TickTraceUIStore& m_TickTraceUIStore;
        BenchUIStore& m_BenchUIStore;

        // Copies a scenario file to the inbox, named like the ones of Tools/game.py so the scenarios run in arrival order.
        auto QueueScenario(const path& file) -> void;

        // Draws the status and the scenario picker of the panel.
        auto DrawScenarios() -> void;

        // Takes the oldest scenario of the inbox (moved to taken/) and starts it. A scenario that does not parse gets a failed report.
        auto TakeScenario(BenchRun& run) -> void;

        // Reads a scenario in the line format Tools/game.py writes (one "key value" per line).
        // return: Empty if it parsed, otherwise what is wrong.
        auto Parse(std::string_view text, BenchScenario& scenario) const -> std::string;

        // Loads the config of the next match, sets its local users and requests the launch.
        auto BeginMatch(BenchRun& run) -> void;

        // Moves the current match one phase forward when its condition is met, or ends it when a timeout passes.
        auto Step(BenchRun& run) -> void;

        // Opens the probe files and starts the TickTrace of a match that asks for them, when its first tick arrives.
        auto StartTools(BenchRun& run) -> void;

        // On every new tick of a running match: remembers the players for the checks, plays the actions, sends the requests and
        // reads the probes.
        // param ticks: Ticks of the match so far.
        auto Sample(BenchRun& run, std::uint64_t ticks) -> void;

        // Gives every player the bench changed back its engine action and stops the TickTrace.
        auto StopTools(BenchRun& run) -> void;

        // return: The address the probe points to now, empty if a pointer of its chain cannot be read or is null.
        auto ResolveProbe(const BenchProbe& probe) -> std::optional<std::uintptr_t>;

        // Sends Leave and waits for the engine to be destroyed.
        // param end: How the match ended, for the report.
        auto Leave(BenchRun& run, std::string_view end) -> void;

        // Collects the log, runs the checks, writes the report and moves to the next match, or closes the scenario after the last one.
        auto EndMatch(BenchRun& run) -> void;

        // Runs the checks of the current match against its report and its last tick.
        auto Check(const BenchRun& run, BenchMatchReport& report) const -> void;

        auto WriteState(BenchRun& run) -> void;
        auto WriteReport(const BenchRun& run, bool isDone) const -> void;

        // Writes the text through a temporary file, so Tools/game.py never reads half a file.
        auto WriteFile(const path& file, const std::string& text) const -> void;
    };
}