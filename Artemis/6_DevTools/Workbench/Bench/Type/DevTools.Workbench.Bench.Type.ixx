export module DevTools.Workbench.Bench.Type;

import Common.Engine.Type;
import Common.Math.Type;
import Control.Action.Type;
import Services.Telemetry.Type;
import Engine.Control.Simulation.Type;
import Bridge.Transport.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Workbench.Simulation.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Control::Action::Type::ClientRequests;
    using Services::Telemetry::Type::PerfSnapshot;
    using Bridge::Transport::Type::BridgeCounters;
    using Bridge::Codec::Tick::Type::Tick;
    using DevTools::Workbench::Simulation::Type::LocalPlayerAction;
    using Engine::Control::Simulation::Type::Roster;

    using Common::Engine::Type::k_MaxTeams;
    using Common::Engine::Type::k_NoHandle;
}

export namespace DevTools::Workbench::Bench::Type
{
    // File next to the DLL that turns the bench on. Its text is the exchange folder of the game watcher (D129).
    inline constexpr std::string_view k_BenchFileName{ "artemis_bench.txt" };

    // Check of a match, from the closed catalog of the bench (D130): a name and its arguments.
    struct BenchCheck
    {
        std::string Name{};
        std::vector<std::string> Arguments{};
    };

    // Players an action or a request of the bench applies to.
    enum class BenchPlayers
    {
        All,
        // Players without a local index: the bots of Artemis.
        Bots,
        // Local users of the split-screen.
        Locals,
    };

    // Test mode of the simulation tab played by some players during a window of ticks (D109, D119).
    struct BenchAction
    {
        BenchPlayers Players{ BenchPlayers::All };
        LocalPlayerAction Mode{ LocalPlayerAction::Engine };
        // Ticks of the match, from 1. ToTick 0 = until the end.
        std::uint64_t FromTick{ 0 };
        std::uint64_t ToTick{ 0 };
    };

    // Client request (next weapon, grenade or zoom level, D120) sent once to some players at a tick of the match.
    struct BenchRequest
    {
        BenchPlayers Players{ BenchPlayers::All };
        ClientRequests Requests{};
        std::uint64_t AtTick{ 0 };
    };

    // Bytes of game memory written to a CSV whenever they change. Development tool for hypotheses of the reverse (D72).
    struct BenchProbe
    {
        std::string Name{};
        // haloreach, mcc or abs: the base the first offset is added to.
        std::string Module{};
        // First offset from the base. Each next one: read a pointer at the address, then add it.
        std::vector<std::uintptr_t> Offsets{};
        std::size_t Size{ 4 };
    };

    // One match of a scenario: what to launch, who plays, how fast, when to stop and what to check at the end.
    struct BenchMatch
    {
        std::string Name{};
        // Recorded match config transplanted at the start (D123).
        std::string Config{};
        // Gametype preset made active before the launch (D264). Empty text removes it, no value leaves the active one.
        std::optional<std::string> Preset{};
        // Map variant (name of a recorded config whose map variant is used) made active before the launch (Q98, D266). Empty text removes
        // it, no value leaves the active one.
        std::optional<std::string> MapVariant{};
        // Local users of the split-screen, the primary one included. Empty leaves the count as it is.
        std::optional<std::size_t> Locals{};
        // Bots of the match (D284): applied at the first tick, and a roster per next round, each one requested while the round before
        // it runs and applied at the end of that round. No roster at all means no bots: the one of the previous match is cleared.
        std::optional<Roster> RosterAtLoad{};
        std::vector<Roster> RoundRosters{};
        // Engine speed during the match. Empty leaves it as it is, 0 = the engine speed.
        std::optional<float> TimeScale{};
        // Limits after which the bench sends Leave (D131). 0 = none: the gametype ends the match.
        std::uint64_t MaxTicks{ 0 };
        double MaxSeconds{ 0.0 };
        std::vector<BenchCheck> Checks{};
        std::vector<BenchAction> Actions{};
        std::vector<BenchRequest> Requests{};
        std::vector<BenchProbe> Probes{};
        // Records the .tdump of the match with the TickDump, and the .sdump too when IsTickDumpFull (the storage folder of the TickDump tool).
        bool IsTickDump{ false };
        bool IsTickDumpFull{ false };
        // Writes every event of the match to a CSV in the events folder of the bench (D104, A14).
        bool IsEvents{ false };
        // Raises the every-tick demand for the whole match (Q101). Off lets a client run the sparse mode during it: the bench then
        // sees only the ticks of the decisions, which its 5 Hz checks and probes do not notice. Off cannot go with tickdump or events.
        bool IsEveryTick{ true };
    };

    // List of matches the bench plays in order (H3). The same list is a rotation or a list of tests (D130).
    struct BenchScenario
    {
        std::string Id{};
        std::string Name{};
        std::vector<BenchMatch> Matches{};
    };

    // Step of the current match.
    enum class BenchPhase
    {
        // No match running: the next one starts on the next update.
        Idle,
        // Launch requested, waiting for the engine start (the request waits for the lobby).
        Launching,
        // Engine started, waiting for the first tick.
        Loading,
        // Ticks arriving.
        Running,
        // Leave sent, waiting for the engine to be destroyed.
        Leaving,
    };

    // Result of one check.
    struct BenchCheckResult
    {
        std::string Name{};
        bool IsOk{ false };
        std::string Detail{};
    };

    // What happened in one match.
    struct BenchMatchReport
    {
        std::size_t Index{ 0 };
        std::string Name{};
        std::string Config{};
        // pending, running, ok, fail.
        std::string Result{ "pending" };
        // How the match ended: gametype, limit, run-timeout, launch-timeout, load-timeout, leave-timeout, destroyed-while-loading, no-config, no-preset.
        std::string End{};
        std::uint64_t Ticks{ 0 };
        double LaunchSeconds{ 0.0 };
        double LoadSeconds{ 0.0 };
        double RunSeconds{ 0.0 };
        std::vector<BenchCheckResult> Checks{};
        // WARNING and ERROR lines of the log during the match.
        std::vector<std::string> Log{};
        // Files written during the match (probes, TickDump), as name and path.
        std::vector<std::pair<std::string, std::string>> Files{};
        // The "perf" JSON object of the report: where the wall clock of the run went. Empty when the match ran no sweeps.
        std::string Perf{};
    };

    // One round start the bench saw (the roster_ok check): the bots per team the roster of the round asks for and the ones the Tick had.
    struct BenchRosterRow
    {
        std::int16_t Round{ 0 };
        std::array<std::size_t, k_MaxTeams> Expected{};
        std::array<std::size_t, k_MaxTeams> Seen{};
        // Bots seen on a team that is not 0 to k_MaxTeams - 1.
        std::size_t SeenElsewhere{ 0 };

        auto IsOk() const -> bool
        {
            return Expected == Seen && SeenElsewhere == 0;
        }
    };

    // What the bench remembers of a player from the first sampled tick where it was alive, for the checks.
    struct BenchPlayerSample
    {
        Vec3 Position{};
        std::uint32_t Weapon{ k_NoHandle };
        // Largest distance from Position on a sampled tick while alive: a player walking in circles still counts as moved.
        float FarthestDistance{ 0.0f };
    };

    // Output of one probe during a match.
    struct BenchProbeState
    {
        std::filesystem::path File{};
        std::vector<std::byte> Last{};
        bool IsReadable{ false };
        std::size_t Rows{ 0 };
    };

    // Progress of the running scenario.
    // note: Bench thread only.
    struct BenchRun
    {
        std::optional<BenchScenario> Scenario{};
        std::vector<BenchMatchReport> Reports{};
        std::size_t MatchIndex{ 0 };
        BenchPhase Phase{ BenchPhase::Idle };
        std::chrono::steady_clock::time_point ScenarioStart{};
        std::chrono::steady_clock::time_point PhaseStart{};
        // Last time the launch was requested for the current match.
        std::chrono::steady_clock::time_point LaunchRequest{};
        // Engine counters when the match began.
        std::uint64_t EngineStarts{ 0 };
        std::uint64_t EngineDestroys{ 0 };
        // Generation of the last tick seen before the start, of the first tick of the match and of the last one.
        std::uint64_t GenerationBefore{ 0 };
        std::uint64_t FirstGeneration{ 0 };
        std::uint64_t LastGeneration{ 0 };
        // Last tick of the match, for the checks.
        std::shared_ptr<const Tick> LastTick{};
        // Log sequence when the match began.
        std::uint64_t LogSequence{ 0 };
        // Telemetry and Lockstep counters when the run began, empty once the perf of the match was closed.
        std::optional<PerfSnapshot> PerfStart{};
        BridgeCounters LockstepStart{};
        // Players seen alive, by handle, and whether any of them held the objective on a sampled tick.
        std::unordered_map<std::uint32_t, BenchPlayerSample> FirstSamples{};
        bool IsObjectiveSeen{ false };
        // Test mode each player plays because of the bench, by player handle (a new player at an index has another handle), and the
        // requests already sent.
        std::unordered_map<std::uint32_t, LocalPlayerAction> AppliedModes{};
        std::vector<bool> SentRequests{};
        // Rounds of the roster check: the last round seen in play, the round rosters requested so far and the rows.
        std::int16_t LastRound{ -1 };
        std::size_t RostersRequested{ 0 };
        std::vector<BenchRosterRow> RosterRows{};
        std::vector<BenchProbeState> Probes{};
        // CSV of the events of the match, and the sequence of the last event written to it.
        std::filesystem::path EventsFile{};
        std::uint32_t EventCursor{ 0 };
        bool IsTickDumpNoted{ false };
        std::chrono::steady_clock::time_point LastStateWrite{};
        std::string Message{};
    };
}