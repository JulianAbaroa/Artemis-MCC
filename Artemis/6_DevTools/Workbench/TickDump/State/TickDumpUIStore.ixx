export module DevTools.Workbench.TickDump.State;

import Common.State.Type;
import Bridge.Transport.Type;
import Blam.Layout.Snapshot.Type;
import Bridge.Codec.Dump.Type;
import DevTools.Workbench.TickDump.Type;
import std;

export namespace DevTools::Workbench::TickDump::State
{
    // State of the TickDump recording (D151): the queue from the AI thread to the dump thread, the requests of the UI, the two files
    // (.tdump and .sdump) and the counters.
    // note: Three threads use it. The AI thread pushes, the dump thread pops and owns the file and the flat buffer, the render
    // thread sends requests and reads the counters, the path and the status.
    class TickDumpUIStore
    {
    private:
        using milliseconds = std::chrono::milliseconds;
        using path = std::filesystem::path;

        using BridgeTick = Contract::Bridge::Type::BridgeTick;

        using SnapshotWriterState = Blam::Layout::Snapshot::Type::SnapshotWriterState;
        using TickDumpWriterState = Bridge::Codec::Dump::Type::TickDumpWriterState;
        using TickDumpEntry = DevTools::Workbench::TickDump::Type::TickDumpEntry;
        using TickDumpStartRequest = DevTools::Workbench::TickDump::Type::TickDumpStartRequest;
        using time_point = std::chrono::steady_clock::time_point;

    public:
        TickDumpUIStore() = default;
        ~TickDumpUIStore() = default;

        TickDumpUIStore(const TickDumpUIStore&) = delete;
        auto operator=(const TickDumpUIStore&) -> TickDumpUIStore& = delete;

        // Asks the dump thread to start a recording.
        // param source: what is recorded, written to the file header.
        // param isSnapshotWritten: Also write the .sdump. A bench match records only the .tdump unless it asks for both.
        auto RequestStart(std::string source, bool isSnapshotWritten) -> void;
        // return: A pending start request, which is cleared.
        auto TakeStartRequest() -> std::optional<TickDumpStartRequest>;
        auto RequestStop() -> void;
        auto TakeStopRequest() -> bool;

        // note: Cleared before the dump thread writes the footer, so the AI thread stops pushing and wakes from a full queue.
        auto SetRecording(bool value) -> void;
        auto IsRecording() const -> bool;

        // Whether the running recording also writes the .sdump. Dump thread only.
        auto SetSnapshotWritten(bool value) -> void;
        auto IsSnapshotWritten() const -> bool;

        // When the previous record was queued (the start of the recording before the first one), to measure the time between records.
        // Dump thread only.
        auto SetLastRecordAt(time_point value) -> void;
        auto GetLastRecordAt() const -> time_point;

        // A recording waiting for the next map load drops every Tick until the first one after the load (D208).
        // note: Set by the render thread before the start request, cleared by the dump thread when that Tick arrives.
        auto SetWaitingForLoad(bool value) -> void;
        auto IsWaitingForLoad() const -> bool;

        // Queues a Tick. Waits while its bytes do not fit under the cap and the recording goes on, so no Tick is lost (D151).
        // An entry alone above the cap still goes in.
        auto Push(TickDumpEntry entry) -> void;
        // return: The oldest queued Tick, or none after the timeout.
        auto WaitPop(milliseconds timeout) -> std::optional<TickDumpEntry>;
        auto GetQueuedTicks() const -> std::size_t;
        // Drops the queued Ticks a previous recording left behind.
        auto ClearQueue() -> void;

        // Buffer the dump thread flattens each Tick into. Dump thread only.
        auto GetFlat() -> BridgeTick&;
        // What the writer of the .tdump keeps between records. Dump thread only.
        auto GetWriterState() -> TickDumpWriterState&;
        // What the writer of the .sdump keeps between records. Dump thread only.
        auto GetSnapshotWriterState() -> SnapshotWriterState&;

        // Dump thread only.
        auto GetFile() -> std::ofstream&;
        auto GetSnapshotFile() -> std::ofstream&;

        auto SetFilePath(path filePath) -> void;
        auto GetFilePath() const -> path;
        auto SetStatus(std::string status) -> void;
        auto GetStatus() const -> std::string;

        auto AddRecordedTick() -> void;
        auto GetRecordedTicks() const -> std::uint64_t;
        auto AddWrittenBytes(std::uint64_t bytes) -> void;
        auto GetWrittenBytes() const -> std::uint64_t;
        auto AddSnapshotBytes(std::uint64_t bytes) -> void;
        auto GetSnapshotBytes() const -> std::uint64_t;
        auto AddDroppedTicks(std::uint64_t ticks) -> void;
        auto GetDroppedTicks() const -> std::uint64_t;
        auto GetQueueWaits() const -> std::uint64_t;
        auto AddEventLoss() -> void;
        auto GetEventLosses() const -> std::uint64_t;
        auto ResetCounters() -> void;

    private:
        // Bytes the queue keeps alive before the AI thread waits (about 2 s of game at x1 for a CTF map). Counted by EstimateBytes, not by
        // Ticks: an entry keeps its Snapshot arena alive, so the count said little about the memory.
        static constexpr std::size_t k_QueueCapacityBytes{ 256ull * 1024 * 1024 };

        // return: What the entry keeps alive: the Tick, the arena and the regions of its Snapshot. Cheap, no walk of the objects.
        auto EstimateBytes(const TickDumpEntry& entry) const -> std::size_t;

        std::atomic<bool> m_IsRecording{};
        std::atomic<bool> m_IsWaitingForLoad{};
        std::atomic<bool> m_IsStopRequested{};
        std::optional<TickDumpStartRequest> m_StartRequest{};
        bool m_IsSnapshotWritten{ true };
        time_point m_LastRecordAt{};

        Common::State::Type::ByteBoundedQueue<TickDumpEntry> m_Queue{ k_QueueCapacityBytes };

        std::unique_ptr<BridgeTick> m_Flat{ std::make_unique<BridgeTick>() };
        TickDumpWriterState m_WriterState{};
        SnapshotWriterState m_SnapshotWriterState{};
        std::ofstream m_File{};
        std::ofstream m_SnapshotFile{};

        path m_FilePath{};
        std::string m_Status{};
        mutable std::mutex m_TextMutex{};

        std::atomic<std::uint64_t> m_RecordedTicks{};
        std::atomic<std::uint64_t> m_WrittenBytes{};
        std::atomic<std::uint64_t> m_SnapshotBytes{};
        std::atomic<std::uint64_t> m_DroppedTicks{};
        std::atomic<std::uint64_t> m_QueueWaits{};
        std::atomic<std::uint64_t> m_EventLosses{};
    };
}