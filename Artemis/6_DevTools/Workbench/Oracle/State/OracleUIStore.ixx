export module DevTools.Workbench.Oracle.State;

import Views.Oracle.Type;
import DevTools.Workbench.Oracle.Type;
import std;

namespace
{
    using DevTools::Workbench::Oracle::Type::k_DefaultEveryTicks;
    using DevTools::Workbench::Oracle::Type::k_DefaultRays;
    using DevTools::Workbench::Oracle::Type::k_RandomSeed;
}

export namespace DevTools::Workbench::Oracle::State
{
    // The settings of the raycast oracle, what it counted and the batches it waits for.
    // note: The settings and the summary text are shared with the render thread. The stats, the pending batches, the random generator and
    // the counters belong to the AI thread alone.
    class OracleUIStore
    {
    private:
        using path = std::filesystem::path;
        using time_point = std::chrono::steady_clock::time_point;

        using OracleStats = Views::Oracle::Type::Stats::OracleStats;
        using PendingBatch = DevTools::Workbench::Oracle::Type::PendingBatch;

    public:
        // Prefix of the keys of this store in the preferences file.
        // note: Only the number of rays and the period are saved. Whether the oracle runs is never saved, so a game always starts with it off.
        static constexpr std::string_view k_OraclePreferencePrefix{ "RaycastOracle_" };

        OracleUIStore() = default;
        ~OracleUIStore() = default;

        OracleUIStore(const OracleUIStore&) = delete;
        auto operator=(const OracleUIStore&) -> OracleUIStore& = delete;

        // Whether the user asked for the oracle. Off by default and never saved.
        auto IsEnabled() const -> bool;
        auto SetEnabled(bool value) -> void;

        // How many rays each batch holds: the rays of the tick first, random ones for the rest.
        auto GetRaysPerBatch() const -> int;

        // note: Clamped to 1 and k_MaxRays.
        auto SetRaysPerBatch(int value) -> void;

        // Every how many ticks a batch is handed over.
        auto GetEveryTicks() const -> int;

        // note: Clamped to 1 and k_MaxEveryTicks.
        auto SetEveryTicks(int value) -> void;

        // Writes the settings as "<prefix><key>=<value>" lines.
        auto Save(std::ostream& stream) const -> void;

        // Applies one saved setting. Ignores unknown keys (an older file may have "Enabled") and values that cannot be read.
        auto Load(std::string_view key, std::string_view value) -> void;

        // Starts a new run: empty stats, no pending batch, the random generator back to its seed.
        // param reportPath: File of the report, empty when there is no storage folder.
        // param header: First lines of the report.
        // note: AI thread.
        auto Reset(path reportPath, std::string header) -> void;

        // note: The four below are AI thread only.
        auto GetStats() -> OracleStats&;
        auto GetPending() -> std::deque<PendingBatch>&;
        auto GetRandom() -> std::mt19937&;
        auto GetReportPath() const -> const path&;
        auto GetHeader() const -> const std::string&;

        // return: The ticks the oracle saw since Reset, including this call.
        auto AdvanceTicks() -> std::uint64_t;

        // return: True the first time a gate bit is marked since Reset.
        // note: AI thread.
        auto MarkLogged(std::uint32_t gate) -> bool;

        auto GetLastSummary() const -> time_point;
        auto SetLastSummary(time_point value) -> void;

        // The last summary line, for the window.
        auto GetSummary() const -> std::string;
        auto SetSummary(std::string value) -> void;

    private:
        std::atomic<bool> m_IsEnabled{ false };
        std::atomic<int> m_RaysPerBatch{ k_DefaultRays };
        std::atomic<int> m_EveryTicks{ k_DefaultEveryTicks };

        OracleStats m_Stats{};
        std::deque<PendingBatch> m_Pending{};
        std::mt19937 m_Random{ k_RandomSeed };
        path m_ReportPath{};
        std::string m_Header{};
        std::uint64_t m_Ticks{ 0 };
        std::uint32_t m_LoggedGates{ 0 };
        time_point m_LastSummary{};

        mutable std::mutex m_Mutex{};
        std::string m_Summary{};
    };
}