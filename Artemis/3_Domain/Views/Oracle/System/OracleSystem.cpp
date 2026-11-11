module Views.Oracle.System;

import Common.Engine.Type;
import Common.Math.Type;
import Blam.MapModel.World.Type;
import Blam.Layout.Collision.Type;
import Blam.Runtime.Object.Type;
import Perception.ObjectGraph.Type;
import Views.Raycast.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::Layout::Collision::Type::Result::CollisionResult;
    using Blam::Layout::Collision::Type::Result::ResultKind;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Views::Oracle::Type::Sample::HitClass;
    using Views::Oracle::Type::Sample::OracleComparison;
    using Views::Oracle::Type::Sample::OracleSample;
    using Views::Oracle::Type::Sample::RayDifference;
    using Views::Oracle::Type::Sample::RaySource;
    using Views::Oracle::Type::Stats::CostHistogram;
    using Views::Oracle::Type::Stats::OracleStats;
    using Views::Oracle::Type::Stats::OracleSummary;
    using Views::Oracle::Type::Stats::SideCounters;
    using Views::Oracle::Type::Stats::WorstDifference;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::RaycastHit;

    using Common::Engine::Type::k_NoHandle;
    using Common::Math::Type::k_TwoPi;
    using Perception::ObjectGraph::Type::k_MaxParentDepth;
    using Views::Oracle::Type::Constant::k_DistanceTolerance;
    using Views::Oracle::Type::Constant::k_HistogramBucketCount;
    using Views::Oracle::Type::Constant::k_HistogramBucketMicroseconds;
    using Views::Oracle::Type::Constant::k_WorstCount;

    constexpr float k_NanosecondsPerMicrosecond{ 1000.0f };

    // Weights of the differences, so a missed hit ranks above a kind, an object, a distance, a material and the flags, in that order.
    constexpr float k_HitScore{ 1000.0f };
    constexpr float k_KindScore{ 500.0f };
    constexpr float k_ObjectScore{ 400.0f };
    constexpr float k_DistanceScore{ 100.0f };
    constexpr float k_DistanceScoreLimit{ 99.0f };
    constexpr float k_MaterialScore{ 10.0f };
    constexpr float k_FlagsScore{ 1.0f };

    // Fills the distance fields of two hits and tells whether they differ.
    auto CompareDistance(RayDifference& difference) -> void
    {
        difference.DistanceDelta = std::abs(difference.EngineDistance - difference.OursDistance);
        difference.IsDistanceDifferent = difference.DistanceDelta > k_DistanceTolerance;
    }

    auto CountIf(bool condition) -> std::uint64_t
    {
        return condition ? 1 : 0;
    }

    auto Score(const RayDifference& difference) -> float
    {
        float score{ 0.0f };

        if (difference.IsHitDifferent) score += k_HitScore;
        if (difference.IsKindDifferent) score += k_KindScore;
        if (difference.IsObjectDifferent) score += k_ObjectScore;
        if (difference.IsDistanceDifferent) score += k_DistanceScore + (std::min)(difference.DistanceDelta, k_DistanceScoreLimit);
        if (difference.IsMaterialDifferent) score += k_MaterialScore;
        if (difference.IsFlagsDifferent) score += k_FlagsScore;

        return score;
    }

    auto RecordSide(SideCounters& side, const RayDifference& difference, const CollisionResult& engine) -> void
    {
        ++side.Rays;
        side.OursHits += CountIf(difference.IsOursHit);

        if (difference.IsEngineHit)
        {
            ++side.EngineHits;
            side.EngineStructureHits += CountIf(engine.Kind == ResultKind::Structure);
            side.EngineInstanceHits += CountIf(engine.Kind == ResultKind::InstancedGeometry);
            side.EngineObjectHits += CountIf(engine.Kind == ResultKind::Object);
        }

        side.HitMismatches += CountIf(difference.IsHitDifferent);
        side.DistanceMismatches += CountIf(difference.IsDistanceDifferent);
        side.KindMismatches += CountIf(difference.IsKindDifferent);
        side.ObjectMismatches += CountIf(difference.IsObjectDifferent);
        side.MaterialMismatches += CountIf(difference.IsMaterialDifferent);
        side.FlagsMismatches += CountIf(difference.IsFlagsDifferent);

        if (difference.IsEngineHit && difference.IsOursHit)
        {
            ++side.BothHit;
            side.SumDistanceDelta += difference.DistanceDelta;
            side.MaxDistanceDelta = (std::max)(side.MaxDistanceDelta, difference.DistanceDelta);
        }
    }

    auto AddCost(CostHistogram& histogram, std::uint32_t nanoseconds) -> void
    {
        const float microseconds{ static_cast<float>(nanoseconds) / k_NanosecondsPerMicrosecond };
        const std::size_t bucket{ (std::min)(static_cast<std::size_t>(microseconds / k_HistogramBucketMicroseconds), k_HistogramBucketCount) };

        ++histogram.Buckets[bucket];
        ++histogram.Count;
        histogram.SumMicroseconds += microseconds;
        histogram.MaxMicroseconds = (std::max)(histogram.MaxMicroseconds, microseconds);
    }

    auto KeepWorst(std::vector<WorstDifference>& worst, const OracleSample& sample, const RayDifference& difference, bool isObjectsSide)
        -> void
    {
        if (!difference.IsCompared || !difference.IsAnyDifferent()) return;

        const float score{ Score(difference) };
        if (worst.size() >= k_WorstCount && score <= worst.back().Score) return;

        const auto position{ std::ranges::find_if(worst, [score](const WorstDifference& other) {
            return other.Score < score;
        }) };
        worst.insert(position, WorstDifference{ .Score = score, .IsObjectsSide = isObjectsSide, .Sample = sample, .Difference = difference });

        if (worst.size() > k_WorstCount) worst.pop_back();
    }

    auto Mean(double sum, std::uint64_t count) -> double
    {
        return count == 0 ? 0.0 : sum / static_cast<double>(count);
    }
}

namespace Views::Oracle::System
{
    auto ClassifyHit(const CollisionResult& result) -> HitClass
    {
        if (!result.IsHit) return HitClass::None;

        return result.Kind == ResultKind::Object ? HitClass::Dynamic : HitClass::Static;
    }

    auto CompareStatic(const OracleSample& sample) -> RayDifference
    {
        RayDifference difference{};
        difference.IsCompared = true;
        difference.IsEngineHit = sample.EngineStatic.IsHit;
        difference.IsOursHit = sample.OursStatic.IsHit;
        difference.EngineDistance = difference.IsEngineHit ? sample.EngineStatic.Fraction * sample.MaxDistance : 0.0f;
        difference.OursDistance = difference.IsOursHit ? sample.OursStatic.Distance : 0.0f;

        if (difference.IsEngineHit != difference.IsOursHit)
        {
            difference.IsHitDifferent = true;
            return difference;
        }

        if (!difference.IsEngineHit) return difference;

        CompareDistance(difference);
        difference.IsKindDifferent = ClassifyHit(sample.EngineStatic) != HitClass::Static;
        difference.IsMaterialDifferent = sample.EngineStatic.Material != sample.OursStatic.Material;
        difference.IsFlagsDifferent = sample.EngineStatic.SurfaceFlags != sample.OursStatic.SurfaceFlags;

        return difference;
    }

    auto CompareObjects(const OracleSample& sample) -> RayDifference
    {
        RayDifference difference{};
        if (!sample.OursTick.has_value()) return difference;

        const RaycastHit& ours{ *sample.OursTick };
        const CollisionResult& engine{ sample.EngineObjects };

        difference.IsCompared = true;
        difference.IsEngineHit = engine.IsHit;
        difference.IsOursHit = ours.Hit;
        difference.EngineDistance = difference.IsEngineHit ? engine.Fraction * sample.MaxDistance : 0.0f;
        difference.OursDistance = difference.IsOursHit ? ours.Distance : 0.0f;

        if (difference.IsEngineHit != difference.IsOursHit)
        {
            difference.IsHitDifferent = true;
            return difference;
        }

        if (!difference.IsEngineHit) return difference;

        CompareDistance(difference);

        const HitClass oursClass{ ours.Kind == HitKind::Dynamic ? HitClass::Dynamic : HitClass::Static };
        difference.IsKindDifferent = ClassifyHit(engine) != oursClass;
        if (difference.IsKindDifferent) return difference;

        if (oursClass == HitClass::Dynamic)
        {
            difference.IsObjectDifferent = engine.ObjectHandle != ours.ObjectHandle;
            return difference;
        }

        difference.IsMaterialDifferent = engine.Material != ours.Material;
        difference.IsFlagsDifferent = engine.SurfaceFlags != ours.SurfaceFlags;

        return difference;
    }

    auto Compare(const OracleSample& sample) -> OracleComparison
    {
        return { .Static = CompareStatic(sample), .Objects = CompareObjects(sample) };
    }

    auto ResolveRootObject(const ObjectTable& objects, std::uint32_t handle) -> std::uint32_t
    {
        std::uint32_t current{ handle };

        for (int depth{ 0 }; depth < k_MaxParentDepth; ++depth)
        {
            const auto found{ objects.find(current) };
            if (found == objects.end()) break;

            const std::uint32_t parent{ found->second.ParentHandle };
            if (parent == k_NoHandle || parent == 0) break;

            current = parent;
        }

        return current;
    }

    auto RandomDirection(std::mt19937& generator) -> Vec3
    {
        std::uniform_real_distribution<float> unit{ 0.0f, 1.0f };

        const float height{ (2.0f * unit(generator)) - 1.0f };
        const float angle{ k_TwoPi * unit(generator) };
        const float ring{ std::sqrt((std::max)(0.0f, 1.0f - (height * height))) };

        return { ring * std::cos(angle), ring * std::sin(angle), height };
    }

    auto Record(OracleStats& stats, const OracleSample& sample, const OracleComparison& comparison) -> void
    {
        if (sample.Source == RaySource::Tick) ++stats.TickRays;
        else ++stats.RandomRays;

        RecordSide(stats.Static, comparison.Static, sample.EngineStatic);

        if (comparison.Objects.IsCompared) RecordSide(stats.Objects, comparison.Objects, sample.EngineObjects);
        else if (sample.EngineObjects.IsHit && sample.EngineObjects.Kind == ResultKind::Object) ++stats.RandomObjectHits;

        AddCost(stats.StaticCost, sample.EngineStaticNanoseconds);
        AddCost(stats.ObjectsCost, sample.EngineObjectsNanoseconds);

        KeepWorst(stats.Worst, sample, comparison.Static, false);
        KeepWorst(stats.Worst, sample, comparison.Objects, true);
    }

    auto RecordLag(OracleStats& stats, std::uint64_t lagTicks) -> void
    {
        ++stats.Outcomes;
        stats.LagSum += lagTicks;
        stats.LagMax = (std::max)(stats.LagMax, lagTicks);
    }

    auto Percentile(const CostHistogram& histogram, double fraction) -> double
    {
        if (histogram.Count == 0) return 0.0;

        const std::uint64_t target{ (std::max)(std::uint64_t{ 1 },
            static_cast<std::uint64_t>(std::ceil(std::clamp(fraction, 0.0, 1.0) * static_cast<double>(histogram.Count)))) };

        std::uint64_t seen{ 0 };
        for (std::size_t bucket{ 0 }; bucket <= k_HistogramBucketCount; ++bucket)
        {
            seen += histogram.Buckets[bucket];
            if (seen < target) continue;

            // The last bucket takes everything above its edge, so the largest value seen is its answer.
            if (bucket == k_HistogramBucketCount) return histogram.MaxMicroseconds;

            return static_cast<double>(bucket + 1) * k_HistogramBucketMicroseconds;
        }

        return histogram.MaxMicroseconds;
    }

    auto Summarize(const OracleStats& stats) -> OracleSummary
    {
        constexpr double k_Median{ 0.5 };
        constexpr double k_Percentile99{ 0.99 };

        OracleSummary summary{};
        summary.StaticMean = Mean(stats.StaticCost.SumMicroseconds, stats.StaticCost.Count);
        summary.StaticP50 = Percentile(stats.StaticCost, k_Median);
        summary.StaticP99 = Percentile(stats.StaticCost, k_Percentile99);
        summary.ObjectsMean = Mean(stats.ObjectsCost.SumMicroseconds, stats.ObjectsCost.Count);
        summary.ObjectsP50 = Percentile(stats.ObjectsCost, k_Median);
        summary.ObjectsP99 = Percentile(stats.ObjectsCost, k_Percentile99);
        summary.LagMean = Mean(static_cast<double>(stats.LagSum), stats.Outcomes);

        return summary;
    }
}