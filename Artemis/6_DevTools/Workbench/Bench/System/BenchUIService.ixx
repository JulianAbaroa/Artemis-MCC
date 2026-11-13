export module DevTools.Workbench.Bench.System;

import Common.Log.System;
import Services.Logs.State;
import Services.Settings.State;
import Services.Telemetry.State;
import Engine.Base.Memory.System;
import Engine.Base.Host.State;
import Engine.Control.Input.State;
import Engine.Base.Lifecycle.State;
import Engine.Base.MapVariant.System;
import Engine.Base.MatchSetup.Type;
import Engine.Base.MatchSetup.System;
import Engine.Base.Preset.System;
import Engine.Match.State;
import Engine.Match.System;
import Bridge.Transport.System;
import Bridge.Codec.Tick.State;
import DevTools.Workbench.TickDump.State;
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
        using TelemetryStore = Services::Telemetry::State::TelemetryStore;

        using MemoryReaderService = Engine::Base::Memory::System::MemoryReaderService;
        using LocalUserStore = Engine::Base::Host::State::LocalUserStore;
        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;
        using VirtualInputStore = Engine::Control::Input::State::VirtualInputStore;
        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using MapVariantDecodeStore = Engine::Match::State::MapVariantDecodeStore;
        using MatchConfigStore = Engine::Match::State::MatchConfigStore;
        using MatchCycleStore = Engine::Match::State::MatchCycleStore;
        using MatchConfigService = Engine::Match::System::MatchConfigService;
        using MapVariantService = Engine::Base::MapVariant::System::MapVariantService;
        using MatchSetup = Engine::Base::MatchSetup::Type::MatchSetup;
        using MatchSetupService = Engine::Base::MatchSetup::System::MatchSetupService;
        using Roster = Engine::Base::MatchSetup::Type::Roster;
        using PresetService = Engine::Base::Preset::System::PresetService;

        using BridgeService = Bridge::Transport::System::BridgeService;

        using TickStore = Bridge::Codec::Tick::State::TickStore;

        using TickDumpUIStore = DevTools::Workbench::TickDump::State::TickDumpUIStore;

        using BenchProbe = DevTools::Workbench::Bench::Type::BenchProbe;
        using BenchScenario = DevTools::Workbench::Bench::Type::BenchScenario;
        using BenchMatchReport = DevTools::Workbench::Bench::Type::BenchMatchReport;
        using BenchRun = DevTools::Workbench::Bench::Type::BenchRun;
        using BenchSetup = DevTools::Workbench::Bench::Type::BenchSetup;
        using BenchUIStore = DevTools::Workbench::Bench::State::BenchUIStore;
        using EveryTickDemandToken = Services::Settings::State::EveryTickDemandToken;

    public:
        BenchUIService(LogsStore& logsStore, Log& log, SettingsStore& settingsStore,
            MemoryReaderService& memoryReaderService, LocalUserStore& localUserStore, PlayerActionStore& playerActionStore,
            VirtualInputStore& virtualInputStore,
            LifecycleStore& lifecycleStore, MatchConfigStore& matchConfigStore, MatchCycleStore& matchCycleStore,
            MatchConfigService& matchConfigService, MapVariantDecodeStore& mapVariantDecodeStore, MatchSetupService& matchSetupService,
            PresetService& presetService, MapVariantService& mapVariantService, TickStore& tickStore, TickDumpUIStore& tickDumpUIStore,
            TelemetryStore& telemetryStore, BridgeService& bridgeService, BenchUIStore& benchUIStore) :
            m_LogsStore(logsStore), m_Log(log), m_SettingsStore(settingsStore), m_TelemetryStore(telemetryStore),
            m_BridgeService(bridgeService),
            m_MemoryReaderService(memoryReaderService), m_LocalUserStore(localUserStore), m_PlayerActionStore(playerActionStore),
            m_VirtualInputStore(virtualInputStore),
            m_LifecycleStore(lifecycleStore), m_MatchConfigStore(matchConfigStore), m_MatchCycleStore(matchCycleStore),
            m_MatchConfigService(matchConfigService), m_MapVariantDecodeStore(mapVariantDecodeStore), m_MatchSetupService(matchSetupService),
            m_PresetService(presetService),
            m_MapVariantService(mapVariantService), m_TickStore(tickStore), m_TickDumpUIStore(tickDumpUIStore),
            m_BenchUIStore(benchUIStore) {}
        ~BenchUIService() = default;

        BenchUIService(const BenchUIService&) = delete;
        auto operator=(const BenchUIService&) -> BenchUIService& = delete;

        // Turns the bench on when the bench file is next to the DLL (the game watcher writes it on deploy) and creates the folders of
        // the exchange. Without the file the bench stays off and Update does nothing.
        auto Initialize() -> void;

        // Advances the running scenario, or takes the next one from the inbox, and writes state.json every second.
        auto Update() -> void;

        // Draws the bench panel of the Simulation tab: one status line (off, waiting or running), the last result and, while a scenario
        // runs, the state of the setup. Claude queues the scenarios with Tools/game.py, so the panel has no controls.
        auto Draw() -> void;

    private:
        LogsStore& m_LogsStore;
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        TelemetryStore& m_TelemetryStore;
        BridgeService& m_BridgeService;
        MemoryReaderService& m_MemoryReaderService;
        LocalUserStore& m_LocalUserStore;
        PlayerActionStore& m_PlayerActionStore;
        VirtualInputStore& m_VirtualInputStore;
        LifecycleStore& m_LifecycleStore;
        MatchConfigStore& m_MatchConfigStore;
        MatchCycleStore& m_MatchCycleStore;
        MatchConfigService& m_MatchConfigService;
        MapVariantDecodeStore& m_MapVariantDecodeStore;
        MatchSetupService& m_MatchSetupService;
        PresetService& m_PresetService;
        MapVariantService& m_MapVariantService;
        TickStore& m_TickStore;
        TickDumpUIStore& m_TickDumpUIStore;
        BenchUIStore& m_BenchUIStore;
        // Held from the start of a match to the end of its tools, so the pipeline runs every tick during the match (Q101).
        EveryTickDemandToken m_EveryTickDemand{};

        // Takes the oldest scenario of the inbox (moved to taken/) and starts it. A scenario that does not parse gets a failed report.
        auto TakeScenario(BenchRun& run) -> void;

        // Reads a scenario in the line format Tools/game.py writes (one "key value" per line).
        // return: Empty if it parsed, otherwise what is wrong.
        auto Parse(std::string_view text, BenchScenario& scenario) const -> std::string;

        // Loads the config of the next match, sets its local users and requests the launch.
        auto BeginMatch(BenchRun& run) -> void;

        // Runs the decodes of .mvar files the match asks for in one phase (Q106), one at a time: reads the file, asks the lifecycle hook to
        // decode it and, when the result arrives, adds a check "decode_mvar" with what the decode gave and how it differs from the map
        // variant of the config to compare with. A request the hook does not answer in time fails the check.
        // param isLobby: The decodes of the lobby (before the launch, the lobby open) or the ones of the match (once it runs).
        // return: Whether a decode of that phase is still to finish.
        auto StepDecodes(BenchRun& run, bool isLobby) -> bool;

        // Moves the current match one phase forward when its condition is met, or ends it when a timeout passes.
        auto Step(BenchRun& run) -> void;

        // Resolves the names of a setup into values. A name that cannot be resolved makes a refused request (the one waiting stays).
        // param defaultBots: The roster of a setup that sets none: nothing, or the empty roster to take the bots away.
        // param request: Out. The setup, when the names were resolved.
        // return: Empty when it was resolved, else the end of the match that has no preset or no map ("no-preset", "no-map").
        auto ResolveSetup(const BenchSetup& setup, const std::optional<Roster>& defaultBots, MatchSetup& request) -> std::string;

        // Resolves a setup and asks for it, as the bench does.
        // return: What ResolveSetup returns.
        auto RequestSetup(const BenchSetup& setup, const std::optional<Roster>& defaultBots) -> std::string;

        // Opens the probe files and starts the TickDump of a match that asks for them, when its first tick arrives.
        auto StartTools(BenchRun& run) -> void;

        // On every new tick of a running match: remembers the players for the checks, plays the actions, sends the requests and
        // reads the probes.
        // param ticks: Ticks of the match so far.
        auto Sample(BenchRun& run, std::uint64_t ticks) -> void;

        // At the first tick of each round in play: records what the engine holds against what the setups asked for up to that round
        // (roster_ok, setup_ok) and asks for the setup of the next round, so it is applied at the end of this one.
        auto UpdateSetups(BenchRun& run) -> void;

        // Takes the photo of the telemetry and Lockstep counters the perf of the match is measured from, when its run begins.
        auto BeginPerf(BenchRun& run) -> void;

        // Takes the closing photo and writes the perf block of the match from the difference. Does nothing if BeginPerf did not run
        // or the block was already written.
        // note: Called when the run ends (limit or timeout) and again by EndMatch, so the time spent leaving is not measured.
        auto EndPerf(BenchRun& run, BenchMatchReport& report) -> void;

        // Gives every player the bench changed back its engine action and stops the TickDump.
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