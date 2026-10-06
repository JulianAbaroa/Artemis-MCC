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

        // Appends an entry. Drops the oldest one if the buffer is over capacity.
        auto PushBack(LogEntry entry) -> void;

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

        const std::size_t m_MaxCapacity{ 500 };
    };
}