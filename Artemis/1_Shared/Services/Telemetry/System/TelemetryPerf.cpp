module Services.Telemetry.System;
import :Perf;

import Services.Telemetry.Type;
import std;

namespace
{
    using Services::Telemetry::Type::PerfSnapshot;
    using Services::Telemetry::Type::k_SweepBucketCount;
    using Services::Telemetry::Type::k_SweepBucketNs;

    // Later minus earlier, 0 if a counter went backwards.
    auto Difference(std::uint64_t later, std::uint64_t earlier) -> std::uint64_t
    {
        return later > earlier ? later - earlier : 0;
    }
}

namespace Services::Telemetry::System
{
    auto SweepBucket(std::uint64_t durationNs) -> std::size_t
    {
        return static_cast<std::size_t>((std::min)(durationNs / k_SweepBucketNs, static_cast<std::uint64_t>(k_SweepBucketCount)));
    }

    auto HistogramPercentileNs(std::span<const std::uint64_t> histogram, double fraction, std::uint64_t maxNs) -> std::uint64_t
    {
        std::uint64_t total{ 0 };
        for (std::uint64_t count : histogram) total += count;
        if (total == 0) return 0;

        const auto rank{ (std::max)(static_cast<std::uint64_t>(std::ceil(fraction * static_cast<double>(total))), std::uint64_t{ 1 }) };

        std::uint64_t seen{ 0 };
        for (std::size_t bucket{ 0 }; bucket < histogram.size(); ++bucket)
        {
            seen += histogram[bucket];
            if (seen < rank) continue;

            const std::uint64_t edge{ bucket >= k_SweepBucketCount ? maxNs : (bucket + 1) * k_SweepBucketNs };
            return maxNs > 0 ? (std::min)(edge, maxNs) : edge;
        }

        return maxNs;
    }

    auto OtherStageNs(std::uint64_t sweepNs, std::span<const std::uint64_t> stageNs) -> std::uint64_t
    {
        std::uint64_t stagesNs{ 0 };
        for (std::uint64_t ns : stageNs) stagesNs += ns;

        return sweepNs > stagesNs ? sweepNs - stagesNs : 0;
    }

    auto Subtract(const PerfSnapshot& later, const PerfSnapshot& earlier) -> PerfSnapshot
    {
        PerfSnapshot result{};
        result.Ticks = Difference(later.Ticks, earlier.Ticks);
        result.EngineNs = Difference(later.EngineNs, earlier.EngineNs);
        result.ReadWaitNs = Difference(later.ReadWaitNs, earlier.ReadWaitNs);
        result.ReadTimeouts = Difference(later.ReadTimeouts, earlier.ReadTimeouts);
        result.Sweeps = Difference(later.Sweeps, earlier.Sweeps);
        result.SweepNs = Difference(later.SweepNs, earlier.SweepNs);
        result.SweepMaxNs = later.SweepMaxNs;
        result.LightTicks = Difference(later.LightTicks, earlier.LightTicks);
        result.LightNs = Difference(later.LightNs, earlier.LightNs);
        result.LightMaxNs = later.LightMaxNs;
        result.ObjectSlotsMax = later.ObjectSlotsMax;
        result.ObjectsMax = later.ObjectsMax;

        for (std::size_t i{ 0 }; i < result.SweepHistogram.size(); ++i)
        {
            result.SweepHistogram[i] = Difference(later.SweepHistogram[i], earlier.SweepHistogram[i]);
        }

        for (std::size_t i{ 0 }; i < result.StageNs.size(); ++i)
        {
            result.StageNs[i] = Difference(later.StageNs[i], earlier.StageNs[i]);
        }

        return result;
    }
}