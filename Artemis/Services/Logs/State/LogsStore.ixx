export module Service.Logs.State;

import Service.Logs.Type;
import std;

export namespace Service::Logs::State
{
    // Thread-safe buffer of the most recent log entries, oldest first.
    class LogsStore
    {
    private:
        using LogEntry = Service::Logs::Type::LogEntry;

    public:
        LogsStore() = default;
        ~LogsStore() = default;

        // Appends an entry and numbers it. Drops the oldest one if the buffer is over capacity.
        auto PushBack(LogEntry entry) -> void;

        // Copies the entries numbered after the given sequence, oldest first.
        // param out: Out. Receives the entries still in the buffer (the ones dropped by the capacity are lost).
        // return: The sequence of the last entry pushed, to pass on the next call.
        auto CopySince(std::uint64_t sequence, std::vector<LogEntry>& out) const -> std::uint64_t;

        // Visits every entry, oldest first.
        // note: Holds the lock during the walk. The callback must not call back into this store.
        auto ForEachLog(std::function<void(const LogEntry&)> callback) const -> void;
        auto ClearLogs() -> void;

        // Returns a copy of the entry at index, oldest first.
        // return: Empty entry if index is out of range.
        auto GetLogAt(std::size_t index) const -> LogEntry;
        auto GetTotalLogs() const -> std::size_t;

        // Removes every entry matching the predicate.
        // note: Same locking rule as ForEachLog.
        auto RemoveIf(std::function<bool(const LogEntry&)> predicate) -> void;

    private:
        std::deque<LogEntry> m_Logs{};
        mutable std::mutex m_Mutex{};
        std::uint64_t m_LastSequence{ 0 };

        const std::size_t m_MaxCapacity{ 500 };
    };
}