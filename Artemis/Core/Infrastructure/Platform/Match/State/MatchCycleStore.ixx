export module Platform.Match.State:MatchCycle;

import std;

export namespace Platform::Match::State
{
    // What Artemis sees of the match cycle of MCC: whether the custom games lobby is open, how many engines started and were
    // destroyed, and the Leave a tool asks for (A15 H3, D129).
    // note: Thread-safe. Written by the lifecycle hooks on the MCC thread, read by the test bench.
    class MatchCycleStore
    {
    public:
        MatchCycleStore() = default;
        ~MatchCycleStore() = default;

        MatchCycleStore(const MatchCycleStore&) = delete;
        auto operator=(const MatchCycleStore&) -> MatchCycleStore& = delete;

        // True while MCC is in PreGame with the custom games lobby screen of Reach open (the carnage report may be on top).
        auto IsLobbyOpen() const -> bool;
        auto SetLobbyOpen(bool value) -> void;

        // Engines started and destroyed since Artemis was injected. A match ended when the destroys catch up with the starts.
        auto AddEngineStart() -> void;
        auto GetEngineStarts() const -> std::uint64_t;
        auto AddEngineDestroy() -> void;
        auto GetEngineDestroys() const -> std::uint64_t;

        // Global of MCC that holds the live engine. Null until the engine interface hook is installed.
        auto SetEngineGlobal(void** global) -> void;
        auto GetEngineGlobal() const -> void**;

        // Leave of the match, sent to the live engine on the next frame of MCC (D131).
        auto RequestLeave() -> void;
        auto TakeLeaveRequest() -> bool;

        // What the lifecycle hook did with the last Leave. Empty before the first one.
        auto GetLastLeave() const -> std::string;
        auto SetLastLeave(std::string result) -> void;

    private:
        mutable std::mutex m_Mutex{};
        std::string m_LastLeave{};
        std::atomic<bool> m_IsLobbyOpen{ false };
        std::atomic<bool> m_IsLeaveRequested{ false };
        std::atomic<std::uint64_t> m_EngineStarts{ 0 };
        std::atomic<std::uint64_t> m_EngineDestroys{ 0 };
        std::atomic<void**> m_EngineGlobal{ nullptr };
    };
}