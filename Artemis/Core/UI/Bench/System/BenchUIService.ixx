export module UI.Bench.System;

import Service.Logs.State;
import Service.Logs.System;
import Service.Settings.State;
import Platform.Memory.System;
import Platform.Host.State;
import Platform.Input.State;
import Platform.Lifecycle.State;
import Platform.Lifecycle.System;
import Platform.Simulation.State;
import Export.Tick.State;
import UI.TickTrace.State;
import UI.Bench.Type;
import UI.Bench.State;
import std;

export namespace UI::Bench::System
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

        using LogsStore = Service::Logs::State::LogsStore;
        using LogsService = Service::Logs::System::LogsService;
        using SettingsStore = Service::Settings::State::SettingsStore;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;
        using LocalUserStore = Platform::Host::State::LocalUserStore;
        using PlayerActionStore = Platform::Input::State::PlayerActionStore;
        using VirtualInputStore = Platform::Input::State::VirtualInputStore;
        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using MatchConfigStore = Platform::Lifecycle::State::MatchConfigStore;
        using MatchCycleStore = Platform::Lifecycle::State::MatchCycleStore;
        using MatchConfigService = Platform::Lifecycle::System::MatchConfigService;
        using BotStore = Platform::Simulation::State::BotStore;

        using TickStore = Export::Tick::State::TickStore;

        using TickTraceUIStore = UI::TickTrace::State::TickTraceUIStore;

        using BenchProbe = UI::Bench::Type::BenchProbe;
        using BenchScenario = UI::Bench::Type::BenchScenario;
        using BenchMatchReport = UI::Bench::Type::BenchMatchReport;
        using BenchRun = UI::Bench::Type::BenchRun;
        using BenchUIStore = UI::Bench::State::BenchUIStore;

    public:
        BenchUIService(LogsStore& logsStore, LogsService& logsService, SettingsStore& settingsStore,
            MemoryReaderService& memoryReaderService, LocalUserStore& localUserStore, PlayerActionStore& playerActionStore,
            VirtualInputStore& virtualInputStore,
            LifecycleStore& lifecycleStore, MatchConfigStore& matchConfigStore, MatchCycleStore& matchCycleStore,
            MatchConfigService& matchConfigService, BotStore& botStore, TickStore& tickStore, TickTraceUIStore& tickTraceUIStore,
            BenchUIStore& benchUIStore) :
            m_LogsStore(logsStore), m_LogsService(logsService), m_SettingsStore(settingsStore),
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
        LogsService& m_LogsService;
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