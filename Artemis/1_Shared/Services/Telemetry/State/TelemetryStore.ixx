export module Services.Telemetry.State;

import Services.Telemetry.Type;
import std;

namespace
{
    using Services::Telemetry::Type::PerfSnapshot;
    using Services::Telemetry::Type::k_MaxStages;
    using Services::Telemetry::Type::k_SweepHistogramSize;
}

export namespace Services::Telemetry::State
{
    // Lock-free counters fed by hooks and threads, plus the rates derived from them.
    // note: TelemetryService::Update resets the window counters and fills the outputs. The cumulative counters (m_Total*, the sweep
    // histogram and the stage times) are never reset: a consumer takes a photo with CopyPerf at the start and another at the end.
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

        // Cumulative counters: same meaning as the window ones above, never reset by Update.
        std::atomic<std::uint64_t> m_TotalTicks{ 0 };
        // note: Includes the Lockstep wait of the bridge, which happens inside the simulation call (it is not subtracted here).
        std::atomic<std::uint64_t> m_TotalEngineNs{ 0 };
        std::atomic<std::uint64_t> m_TotalReadWaitNs{ 0 };
        std::atomic<std::uint64_t> m_TotalReadTimeouts{ 0 };
        std::atomic<std::uint64_t> m_TotalSweeps{ 0 };
        std::atomic<std::uint64_t> m_TotalSweepNs{ 0 };
        // Longest sweep since the last CopyPerf.
        std::atomic<std::uint64_t> m_SweepMaxNs{ 0 };
        // Sweeps by duration: buckets of k_SweepBucketNs and a last overflow bucket.
        std::array<std::atomic<std::uint64_t>, k_SweepHistogramSize> m_SweepHistogram{};
        // Time of each stage of the sweep summed over all sweeps, in nanoseconds.
        std::array<std::atomic<std::uint64_t>, k_MaxStages> m_TotalStageNs{};

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

        // Records one AI sweep: its duration and the time of each of its stages.
        // param stageNs: Time of each stage in the order of the registered names. Entries past k_MaxStages are ignored.
        auto RecordSweep(std::uint64_t durationNs, std::span<const std::uint64_t> stageNs) -> void;

        // Registers the names of the stages RecordSweep receives, once, before the first sweep.
        // note: The names must outlive the store (string literals). Not thread-safe: the writer is the thread that records the sweeps.
        auto SetStageNames(std::span<const std::string_view> names) -> void;

        // Names registered with SetStageNames.
        auto GetStageNames() const -> std::span<const std::string_view>;

        // Photo of the cumulative counters.
        // note: Restarts SweepMaxNs, so the maximum of a photo is the one since the previous photo. One consumer at a time.
        auto CopyPerf(PerfSnapshot& out) -> void;

        // Counts one presented frame.
        auto RecordPresent() -> void;

    private:
        std::array<std::string_view, k_MaxStages> m_StageNames{};
        std::size_t m_StageCount{ 0 };
    };
}