export module UI.Bench.Type;

import Common.Engine.Type;
import Common.Math.Type;
import Platform.Input.Type;
import Export.Tick.Type;
import UI.Simulation.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Platform::Input::Type::ClientRequests;
    using Export::Tick::Type::Tick;
    using UI::Simulation::Type::LocalPlayerAction;

    using Common::Engine::Type::k_NoHandle;
}

export namespace UI::Bench::Type
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
        // Local users of the split-screen, the primary one included. Empty leaves the count as it is.
        std::optional<std::size_t> Locals{};
        // Team of each bot added after the start. Empty team = the one the engine picks.
        std::vector<std::optional<std::int8_t>> Bots{};
        // Engine speed during the match. Empty leaves it as it is, 0 = the engine speed.
        std::optional<float> TimeScale{};
        // Limits after which the bench sends Leave (D131). 0 = none: the gametype ends the match.
        std::uint64_t MaxTicks{ 0 };
        double MaxSeconds{ 0.0 };
        std::vector<BenchCheck> Checks{};
        std::vector<BenchAction> Actions{};
        std::vector<BenchRequest> Requests{};
        std::vector<BenchProbe> Probes{};
        // Records a TickTrace of the match (the storage folder of the TickTrace tool).
        bool IsTickTrace{ false };
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
        // How the match ended: gametype, limit, run-timeout, launch-timeout, load-timeout, leave-timeout, destroyed-while-loading, no-config.
        std::string End{};
        std::uint64_t Ticks{ 0 };
        double LaunchSeconds{ 0.0 };
        double LoadSeconds{ 0.0 };
        double RunSeconds{ 0.0 };
        std::vector<BenchCheckResult> Checks{};
        // WARNING and ERROR lines of the log during the match.
        std::vector<std::string> Log{};
        // Files written during the match (probes, TickTrace), as name and path.
        std::vector<std::pair<std::string, std::string>> Files{};
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
        // Players seen alive, by handle, and whether any of them held the objective on a sampled tick.
        std::unordered_map<std::uint32_t, BenchPlayerSample> FirstSamples{};
        bool IsObjectiveSeen{ false };
        // Test mode each player index plays because of the bench, and the requests already sent.
        std::unordered_map<std::size_t, LocalPlayerAction> AppliedModes{};
        std::vector<bool> SentRequests{};
        std::vector<BenchProbeState> Probes{};
        bool IsTickTraceNoted{ false };
        std::chrono::steady_clock::time_point LastStateWrite{};
        std::string Message{};
    };
}