export module Platform.Lifecycle.State:MatchConfig;

import Platform.Lifecycle.Type;
import std;

export namespace Platform::Lifecycle::State
{
    // Match configuration the next engine start transplants, the name the next start is recorded under, what the last starts did
    // (D123), and the request to launch the match of the lobby (D124).
    // note: Thread-safe. Written by the UI thread, taken by the start hook on the MCC thread.
    class MatchConfigStore
    {
    private:
        using MatchConfig = Platform::Lifecycle::Type::MatchConfig;

    public:
        MatchConfigStore() = default;
        ~MatchConfigStore() = default;

        MatchConfigStore(const MatchConfigStore&) = delete;
        auto operator=(const MatchConfigStore&) -> MatchConfigStore& = delete;

        // Recorded configuration the next engine start transplants, or null to start the match of MCC unchanged.
        auto GetPending() const -> std::shared_ptr<const MatchConfig>;
        auto SetPending(std::shared_ptr<const MatchConfig> config) -> void;

        // Takes the pending configuration, so it applies to one start only.
        // return: The configuration, or null if none is pending.
        auto TakePending() -> std::shared_ptr<const MatchConfig>;

        // Name the next engine start is recorded under. Empty if the next start is not recorded.
        auto GetRecordName() const -> std::string;
        auto SetRecordName(std::string name) -> void;

        // Takes the record name, so one start only is recorded.
        // return: The name, empty if none is set.
        auto TakeRecordName() -> std::string;

        // Name of the configuration the last transplant applied. Empty before the first one.
        auto GetLastApplied() const -> std::string;
        auto SetLastApplied(std::string name) -> void;

        // File the last record wrote. Empty before the first one.
        auto GetLastRecorded() const -> std::string;
        auto SetLastRecorded(std::string file) -> void;

        // Whether a launch from the lobby is requested and the lifecycle hook did not take it yet (D124).
        auto IsLaunchRequested() const -> bool;
        auto SetLaunchRequested(bool value) -> void;

        // Takes the launch request, so it launches one match only.
        // return: Whether a launch was requested.
        auto TakeLaunchRequest() -> bool;

        // What the lifecycle hook did with the last launch request. Empty before the first one.
        auto GetLastLaunch() const -> std::string;
        auto SetLastLaunch(std::string result) -> void;

    private:
        mutable std::mutex m_Mutex{};
        std::shared_ptr<const MatchConfig> m_Pending{};
        std::string m_RecordName{};
        std::string m_LastApplied{};
        std::string m_LastRecorded{};
        std::string m_LastLaunch{};
        std::atomic<bool> m_IsLaunchRequested{ false };
    };
}