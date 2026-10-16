export module Service.Telemetry.State;

import std;

export namespace Service::Telemetry::State
{
    // Lock-free counters fed by hooks and threads, plus the rates derived from them.
    // note: TelemetryService::Update resets the counters and fills the outputs.
    class TelemetryStore
    {
    public:
        TelemetryStore() = default;
        ~TelemetryStore() = default;

        // Ticks skipped since the last update.
        std::atomic<std::uint64_t> m_Dropped{ 0 };
        // Output. Ticks skipped in the last window.
        std::atomic<std::uint32_t> m_DroppedOut{ 0 };

        // Calls to the simulation tick hook since the last update.
        std::atomic<std::uint64_t> m_SimCalls{ 0 };
        // Ticks advanced by those calls.
        std::atomic<std::uint64_t> m_Ticks{ 0 };
        // Engine time spent in those calls, in nanoseconds. Excludes the time waiting for the AI read.
        std::atomic<std::uint64_t> m_SimNs{ 0 };
        // Time the game thread waited for the AI thread to read a tick, in nanoseconds.
        std::atomic<std::uint64_t> m_ReadWaitNs{ 0 };
        // Ticks whose read wait timed out.
        std::atomic<std::uint64_t> m_ReadTimeouts{ 0 };
        // AI sweeps since the last update. A sweep is one full AI thread pass for a tick.
        std::atomic<std::uint64_t> m_Sweeps{ 0 };
        // Total time spent in those sweeps, in nanoseconds.
        std::atomic<std::uint64_t> m_SweepNs{ 0 };
        // Frames presented since the last update.
        std::atomic<std::uint64_t> m_Presents{ 0 };

        // Output. Ticks per second.
        std::atomic<float> m_TickHz{ 0.0f };
        // Output. Frames per second.
        std::atomic<float> m_PresentHz{ 0.0f };
        // Output. Average logic ticks per simulation call (one call per frame).
        std::atomic<float> m_TicksPerCall{ 0.0f };
        // Output. Average engine time per logic tick, in milliseconds.
        // note: Per tick, not per call. A call runs several ticks when the game goes faster than the frame rate.
        std::atomic<float> m_SimMs{ 0.0f };
        // Output. Average time the game thread waited per tick for the AI read, in milliseconds.
        std::atomic<float> m_ReadWaitMs{ 0.0f };
        // Output. Read waits that timed out in the last window.
        std::atomic<std::uint32_t> m_ReadTimeoutsOut{ 0 };
        // Output. Average sweep time, in milliseconds.
        std::atomic<float> m_SweepMs{ 0.0f };

        // Adds ticks that were skipped without being processed.
        auto RecordDroppedTicks(std::uint64_t dropped) -> void;

        // Records one call of the simulation tick hook.
        // param ticks: Ticks the call ran. A negative value counts as 0.
        // param durationNs: Time spent in the call.
        auto RecordSimTick(int ticks, std::uint64_t durationNs) -> void;

        // Records one wait of the game thread for the AI read of a tick.
        // param isTimedOut: Whether the wait ended by timeout.
        auto RecordReadWait(std::uint64_t waitNs, bool isTimedOut) -> void;

        // Records the duration of one AI sweep.
        auto RecordSweepTime(std::uint64_t durationNs) -> void;

        // Counts one presented frame.
        auto RecordPresent() -> void;
    };
}