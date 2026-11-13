export module DevTools.Workbench.Bench.State;

import DevTools.Workbench.Bench.Type;
import std;

export namespace DevTools::Workbench::Bench::State
{
    // State of the in-game test bench (D129): the exchange folder of the game watcher and the progress of the running scenario.
    // note: The folder is set once, before the first update, and other threads read it only after IsEnabled is true. The run is
    // bench thread only. The status is read by any thread.
    class BenchUIStore
    {
    private:
        using path = std::filesystem::path;

        using BenchLastResult = DevTools::Workbench::Bench::Type::BenchLastResult;
        using BenchRun = DevTools::Workbench::Bench::Type::BenchRun;

    public:
        BenchUIStore() = default;
        ~BenchUIStore() = default;

        BenchUIStore(const BenchUIStore&) = delete;
        auto operator=(const BenchUIStore&) -> BenchUIStore& = delete;

        // Exchange folder of the game watcher. Empty while the bench is off.
        auto SetFolder(path folder) -> void;
        auto GetFolder() const -> const path&;
        auto IsEnabled() const -> bool;

        // Whether a scenario is running. Written by the bench thread, read by the UI.
        auto IsScenarioRunning() const -> bool;
        auto SetScenarioRunning(bool value) -> void;

        // note: Bench thread only.
        auto GetRun() -> BenchRun&;

        // One line about what the bench is doing, for the user.
        auto SetStatus(std::string status) -> void;
        auto GetStatus() const -> std::string;

        // How the last finished scenario ended. Written by the bench thread when it writes the final report, read by the UI.
        // return: Nothing until a scenario has finished.
        auto SetLastResult(BenchLastResult result) -> void;
        auto GetLastResult() const -> std::optional<BenchLastResult>;

    private:
        path m_Folder{};
        std::atomic<bool> m_IsEnabled{ false };
        std::atomic<bool> m_IsScenarioRunning{ false };
        BenchRun m_Run{};
        mutable std::mutex m_Mutex{};
        std::string m_Status{};
        std::optional<BenchLastResult> m_LastResult{};
    };
}