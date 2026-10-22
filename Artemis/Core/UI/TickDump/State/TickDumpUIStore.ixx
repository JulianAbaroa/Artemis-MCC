export module UI.TickDump.State;

import Platform.Bridge.Type;
import UI.TickDump.Type;
import std;

export namespace UI::TickDump::State
{
    // State of the TickDump recording (D151): the queue from the AI thread to the dump thread, the requests of the UI, the file
    // and the counters.
    // note: Three threads use it. The AI thread pushes, the dump thread pops and owns the file and the flat buffer, the render
    // thread sends requests and reads the counters, the path and the status.
    class TickDumpUIStore
    {
    private:
        using milliseconds = std::chrono::milliseconds;
        using path = std::filesystem::path;

        using BridgeTick = Platform::Bridge::Type::BridgeTick;
        using BridgeEntity = Platform::Bridge::Type::BridgeEntity;

        using TickDumpEntry = UI::TickDump::Type::TickDumpEntry;

    public:
        TickDumpUIStore() = default;
        ~TickDumpUIStore() = default;

        TickDumpUIStore(const TickDumpUIStore&) = delete;
        auto operator=(const TickDumpUIStore&) -> TickDumpUIStore& = delete;

        // Asks the dump thread to start a recording.
        // param source: what is recorded, written to the file header.
        auto RequestStart(std::string source) -> void;
        // return: The source of a pending start request, which is cleared.
        auto TakeStartRequest() -> std::optional<std::string>;
        auto RequestStop() -> void;
        auto TakeStopRequest() -> bool;

        // note: Cleared before the dump thread writes the footer, so the AI thread stops pushing and wakes from a full queue.
        auto SetRecording(bool value) -> void;
        auto IsRecording() const -> bool;

        // Queues a Tick. Waits while the queue is full and the recording goes on, so no Tick is lost (D151).
        auto Push(TickDumpEntry entry) -> void;
        // return: The oldest queued Tick, or none after the timeout.
        auto WaitPop(milliseconds timeout) -> std::optional<TickDumpEntry>;
        auto GetQueuedTicks() const -> std::size_t;
        // Drops the queued Ticks a previous recording left behind.
        auto ClearQueue() -> void;

        // Buffer the dump thread flattens each Tick into. Dump thread only.
        auto GetFlat() -> BridgeTick&;
        // Entity rows of the previous record, by handle, the base of the next delta. Dump thread only.
        auto GetPreviousEntities() -> std::vector<BridgeEntity>&;
        // Records written since the last keyframe. Dump thread only.
        auto GetRecordsSinceKeyframe() const -> std::uint32_t;
        auto SetRecordsSinceKeyframe(std::uint32_t records) -> void;
        // Sequence of the newest event already written, to write each event once. Dump thread only.
        auto GetLastEventSequence() const -> std::uint32_t;
        auto SetLastEventSequence(std::uint32_t sequence) -> void;

        // Dump thread only.
        auto GetFile() -> std::ofstream&;

        auto SetFilePath(path filePath) -> void;
        auto GetFilePath() const -> path;
        auto SetStatus(std::string status) -> void;
        auto GetStatus() const -> std::string;

        auto AddRecordedTick() -> void;
        auto GetRecordedTicks() const -> std::uint64_t;
        auto AddWrittenBytes(std::uint64_t bytes) -> void;
        auto GetWrittenBytes() const -> std::uint64_t;
        auto AddDroppedTicks(std::uint64_t ticks) -> void;
        auto GetDroppedTicks() const -> std::uint64_t;
        auto GetQueueWaits() const -> std::uint64_t;
        auto AddEventLoss() -> void;
        auto GetEventLosses() const -> std::uint64_t;
        auto ResetCounters() -> void;

    private:
        // Ticks the queue holds before the AI thread waits: 2 s of game at x1. Each one keeps a whole Tick alive.
        static constexpr std::size_t k_QueueCapacity{ 120 };

        std::atomic<bool> m_IsRecording{};
        std::atomic<bool> m_IsStopRequested{};
        std::optional<std::string> m_StartRequest{};

        std::deque<TickDumpEntry> m_Queue{};
        mutable std::mutex m_QueueMutex{};
        std::condition_variable m_NotEmpty{};
        std::condition_variable m_NotFull{};

        std::unique_ptr<BridgeTick> m_Flat{ std::make_unique<BridgeTick>() };
        std::vector<BridgeEntity> m_PreviousEntities{};
        std::uint32_t m_RecordsSinceKeyframe{};
        std::uint32_t m_LastEventSequence{};
        std::ofstream m_File{};

        path m_FilePath{};
        std::string m_Status{};
        mutable std::mutex m_TextMutex{};

        std::atomic<std::uint64_t> m_RecordedTicks{};
        std::atomic<std::uint64_t> m_WrittenBytes{};
        std::atomic<std::uint64_t> m_DroppedTicks{};
        std::atomic<std::uint64_t> m_QueueWaits{};
        std::atomic<std::uint64_t> m_EventLosses{};
    };
}