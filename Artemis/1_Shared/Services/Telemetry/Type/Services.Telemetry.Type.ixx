export module Services.Telemetry.Type;

import std;

export namespace Services::Telemetry::Type
{
    // Width of one bucket of the sweep duration histogram, in nanoseconds.
    inline constexpr std::uint64_t k_SweepBucketNs{ 25'000 };
    // Buckets that cover [0, k_SweepBucketCount * k_SweepBucketNs), 6.4 ms.
    inline constexpr std::size_t k_SweepBucketCount{ 256 };
    // Buckets plus the overflow bucket (the last one) for sweeps at or above the covered range.
    inline constexpr std::size_t k_SweepHistogramSize{ k_SweepBucketCount + 1 };
    // Capacity of the per-stage arrays. Stage names are registered up to it.
    inline constexpr std::size_t k_MaxStages{ 32 };

    // Photo of the cumulative counters of TelemetryStore. The difference of two photos is what happened between them.
    struct PerfSnapshot
    {
        std::uint64_t Ticks{ 0 };
        // Engine time of the simulation calls, without the wait for the AI read. It still includes the Lockstep wait.
        std::uint64_t EngineNs{ 0 };
        std::uint64_t ReadWaitNs{ 0 };
        std::uint64_t ReadTimeouts{ 0 };
        std::uint64_t Sweeps{ 0 };
        std::uint64_t SweepNs{ 0 };
        // Longest sweep since the previous photo. It is not a difference: the store restarts it when a photo is taken.
        std::uint64_t SweepMaxNs{ 0 };
        std::array<std::uint64_t, k_SweepHistogramSize> SweepHistogram{};
        // Time of each stage summed over the sweeps, in the order of the registered names.
        std::array<std::uint64_t, k_MaxStages> StageNs{};
    };
}