export module Services.Telemetry.System:Perf;

import Services.Telemetry.Type;
import std;

export namespace Services::Telemetry::System
{
    // Index of the histogram bucket a sweep of that duration falls in. The last index is the overflow bucket.
    auto SweepBucket(std::uint64_t durationNs) -> std::size_t;

    // Duration below which the given fraction of the sweeps of a histogram ended.
    // param fraction: 0 to 1. 0.5 is the median.
    // param maxNs: Longest sweep of the same period. It caps the result and stands for the overflow bucket.
    // return: The upper edge of the bucket that holds the rank, in nanoseconds. 0 for an empty histogram.
    auto HistogramPercentileNs(std::span<const std::uint64_t> histogram, double fraction, std::uint64_t maxNs) -> std::uint64_t;

    // Time of a sweep that no stage covers (the read handoff, telemetry): the sweep minus its stages, never negative.
    auto OtherStageNs(std::uint64_t sweepNs, std::span<const std::uint64_t> stageNs) -> std::uint64_t;

    // What happened between two photos of the cumulative counters.
    // note: The maximum is the one of the later photo.
    auto Subtract(const Type::PerfSnapshot& later, const Type::PerfSnapshot& earlier) -> Type::PerfSnapshot;
}