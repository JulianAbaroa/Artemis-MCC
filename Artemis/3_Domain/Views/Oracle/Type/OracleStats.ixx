export module Views.Oracle.Type:Stats;

import :Constant;
import :Sample;

import std;

namespace
{
    using Views::Oracle::Type::Constant::k_HistogramBucketCount;
    using Views::Oracle::Type::Sample::OracleSample;
    using Views::Oracle::Type::Sample::RayDifference;
}

export namespace Views::Oracle::Type::Stats
{
    // What one of the two comparisons counted.
    struct SideCounters
    {
        std::uint64_t Rays{ 0 };
        std::uint64_t EngineHits{ 0 };
        std::uint64_t OursHits{ 0 };

        // Engine hits by kind of result: the structure, the instanced geometry and an object.
        std::uint64_t EngineStructureHits{ 0 };
        std::uint64_t EngineInstanceHits{ 0 };
        std::uint64_t EngineObjectHits{ 0 };

        std::uint64_t HitMismatches{ 0 };
        std::uint64_t DistanceMismatches{ 0 };
        std::uint64_t KindMismatches{ 0 };
        std::uint64_t ObjectMismatches{ 0 };
        std::uint64_t MaterialMismatches{ 0 };
        std::uint64_t FlagsMismatches{ 0 };

        // Rays where both hit: how many, and the sum and the largest of the distance difference in meters.
        std::uint64_t BothHit{ 0 };
        double SumDistanceDelta{ 0.0 };
        float MaxDistanceDelta{ 0.0f };
    };

    // How long the engine took, in buckets of microseconds.
    struct CostHistogram
    {
        std::array<std::uint32_t, k_HistogramBucketCount + 1> Buckets{};
        std::uint64_t Count{ 0 };
        double SumMicroseconds{ 0.0 };
        float MaxMicroseconds{ 0.0f };
    };

    // One of the worst differences: the ray, both answers and how they differ.
    struct WorstDifference
    {
        float Score{ 0.0f };

        // True when it is the comparison with objects, false for the one without.
        bool IsObjectsSide{ false };
        OracleSample Sample{};
        RayDifference Difference{};
    };

    // Everything the oracle counted since it was turned on.
    struct OracleStats
    {
        SideCounters Static{};
        SideCounters Objects{};

        std::uint64_t TickRays{ 0 };
        std::uint64_t RandomRays{ 0 };

        // Rays the game thread did not trace because their values were not valid. They are in no other counter.
        std::uint64_t SkippedRays{ 0 };

        // Random rays where the engine hit an object: they have no answer of Artemis to compare with.
        std::uint64_t RandomObjectHits{ 0 };

        // Outcomes read, and outcomes of a batch the oracle no longer waited for.
        std::uint64_t Outcomes{ 0 };
        std::uint64_t StaleOutcomes{ 0 };

        // Lag between handing a batch over and the engine tracing it, in ticks of the game thread.
        std::uint64_t LagSum{ 0 };
        std::uint64_t LagMax{ 0 };

        CostHistogram StaticCost{};
        CostHistogram ObjectsCost{};

        // Sorted from the worst, at most k_WorstCount.
        std::vector<WorstDifference> Worst{};
    };

    // The numbers of the log line, from OracleStats. Costs are in microseconds.
    struct OracleSummary
    {
        double StaticMean{ 0.0 };
        double StaticP50{ 0.0 };
        double StaticP99{ 0.0 };
        double ObjectsMean{ 0.0 };
        double ObjectsP50{ 0.0 };
        double ObjectsP99{ 0.0 };
        double LagMean{ 0.0 };
    };
}