export module UI.TickTrace.State;

import UI.TickTrace.Type;
import std;

export namespace UI::TickTrace::State
{
    // State of the tick trace recording: the flag, the bytes waiting to be written, the open file and the counters.
    // note: The AI thread appends records and the UI thread writes them. The pending clock is AI thread only, the file UI thread only.
    class TickTraceUIStore
    {
    private:
        using steady_clock = std::chrono::steady_clock;
        using path = std::filesystem::path;

        using TickClock = UI::TickTrace::Type::TickClock;

    public:
        TickTraceUIStore() = default;
        ~TickTraceUIStore() = default;

        TickTraceUIStore(const TickTraceUIStore&) = delete;
        auto operator=(const TickTraceUIStore&) -> TickTraceUIStore& = delete;

        // note: Takes the pending mutex, so no record is appended after the flag goes false.
        auto SetRecording(bool value) -> void;
        auto IsRecording() const -> bool;

        // Start or stop asked for by another thread (the test bench, D129). The UI thread takes them in Update, where the file lives.
        auto RequestStart() -> void;
        auto TakeStartRequest() -> bool;
        auto RequestStop() -> void;
        auto TakeStopRequest() -> bool;

        // Clock of the tick being processed, from CaptureClock to Record.
        auto SetPendingClock(const TickClock& clock) -> void;
        auto GetPendingClock() const -> TickClock;

        // Adds the bytes of one tick to the pending buffer.
        // return: False if the recording stopped, the bytes are dropped.
        auto Append(std::span<const std::byte> bytes) -> bool;

        // Moves the pending bytes out.
        // param out: Out. Receives the pending bytes, the buffer is left empty.
        auto TakePending(std::vector<std::byte>& out) -> void;

        auto AddRecordedTick() -> void;
        auto GetRecordedTicks() const -> std::uint64_t;

        auto AddWrittenBytes(std::uint64_t bytes) -> void;
        auto GetWrittenBytes() const -> std::uint64_t;

        // Start of the recording, the zero of TickRecord::RealTimeNs.
        // note: Set before SetRecording(true), which publishes it to the AI thread.
        auto SetStartTime(steady_clock::time_point time) -> void;
        auto GetStartTime() const -> steady_clock::time_point;

        auto GetFile() -> std::ofstream&;

        auto SetFilePath(path filePath) -> void;
        auto GetFilePath() const -> const path&;

        // Last message for the user, such as why the recording could not start.
        auto SetStatus(std::string status) -> void;
        auto GetStatus() const -> const std::string&;

        // Clears the counters for a new recording.
        auto ResetCounters() -> void;

    private:
        bool m_IsRecording{ false };
        std::atomic<bool> m_IsStartRequested{ false };
        std::atomic<bool> m_IsStopRequested{ false };
        std::vector<std::byte> m_Pending{};
        mutable std::mutex m_PendingMutex{};

        TickClock m_PendingClock{};
        steady_clock::time_point m_StartTime{};

        std::atomic<std::uint64_t> m_RecordedTicks{ 0 };
        std::atomic<std::uint64_t> m_WrittenBytes{ 0 };

        std::ofstream m_File{};
        path m_FilePath{};
        std::string m_Status{};
    };
}