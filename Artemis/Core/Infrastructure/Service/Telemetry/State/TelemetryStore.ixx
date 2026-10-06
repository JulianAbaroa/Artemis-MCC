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
        // Total time spent in those calls, in nanoseconds.
        std::atomic<std::uint64_t> m_SimNs{ 0 };
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
        // Output. Average time of a simulation tick call, in milliseconds.
        std::atomic<float> m_SimMs{ 0.0f };
        // Output. Average sweep time, in milliseconds.
        std::atomic<float> m_SweepMs{ 0.0f };

        // Adds ticks that were skipped without being processed.
        auto RecordDroppedTicks(std::uint64_t dropped) -> void;

        // Records one call of the simulation tick hook.
        // param ticks: Ticks advanced by the call. A negative value counts as 0.
        // param durationNs: Time spent in the call.
        auto RecordSimTick(int ticks, std::uint64_t durationNs) -> void;

        // Records the duration of one AI sweep.
        auto RecordSweepTime(std::uint64_t durationNs) -> void;

        // Counts one presented frame.
        auto RecordPresent() -> void;
    };
}