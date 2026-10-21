export module UI.Bench.State;

import UI.Bench.Type;
import std;

export namespace UI::Bench::State
{
    // State of the in-game test bench (D129): the exchange folder of the game watcher and the progress of the running scenario.
    // note: The folder is set once, before the first update, and other threads read it only after IsEnabled is true. The run is
    // bench thread only. The status is read by any thread.
    class BenchUIStore
    {
    private:
        using path = std::filesystem::path;

        using BenchRun = UI::Bench::Type::BenchRun;

    public:
        BenchUIStore() = default;
        ~BenchUIStore() = default;

        BenchUIStore(const BenchUIStore&) = delete;
        auto operator=(const BenchUIStore&) -> BenchUIStore& = delete;

        // Exchange folder of the game watcher. Empty while the bench is off.
        auto SetFolder(path folder) -> void;
        auto GetFolder() const -> const path&;
        auto IsEnabled() const -> bool;

        // note: Bench thread only.
        auto GetRun() -> BenchRun&;

        // One line about what the bench is doing, for the user.
        auto SetStatus(std::string status) -> void;
        auto GetStatus() const -> std::string;

        // Scenario files the Simulation tab offers (scenarios/ of the exchange folder) and the one selected.
        // note: Render thread only.
        auto GetScenarioFiles() -> std::vector<path>&;
        auto SetSelectedScenario(std::size_t index) -> void;
        auto GetSelectedScenario() const -> std::size_t;

    private:
        path m_Folder{};
        std::atomic<bool> m_IsEnabled{ false };
        BenchRun m_Run{};
        mutable std::mutex m_Mutex{};
        std::string m_Status{};
        std::vector<path> m_ScenarioFiles{};
        std::size_t m_SelectedScenario{ 0 };
    };
}