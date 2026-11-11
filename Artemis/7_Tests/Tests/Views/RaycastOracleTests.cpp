#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Blam.MapModel.World.Type;
import Blam.Layout.Collision.Type;
import Blam.Runtime.Object.Type;
import Views.Raycast.Type;
import Views.Oracle.Type;
import Views.Oracle.System;
import std;

// Comparison, counters, summary and report of the raycast oracle (R3, D272), with made-up engine and Artemis answers: no engine call.

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Math::Type::Vec3;
    using Blam::MapModel::World::Type::Raycast::Hit;
    using Blam::Layout::Collision::Type::Result::CollisionResult;
    using Blam::Layout::Collision::Type::Result::ResultKind;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::RaycastHit;
    using Views::Oracle::Type::Constant::k_DistanceTolerance;
    using Views::Oracle::Type::Constant::k_WorstCount;
    using Views::Oracle::Type::Sample::HitClass;
    using Views::Oracle::Type::Sample::OracleComparison;
    using Views::Oracle::Type::Sample::OracleSample;
    using Views::Oracle::Type::Sample::RayDifference;
    using Views::Oracle::Type::Sample::RaySource;
    using Views::Oracle::Type::Stats::CostHistogram;
    using Views::Oracle::Type::Stats::OracleStats;
    using Views::Oracle::Type::Stats::OracleSummary;
    using namespace Views::Oracle::System;

    constexpr float k_Length{ 50.0f };

    // A ray of 50 m along X.
    auto MakeSample() -> OracleSample
    {
        OracleSample sample{};
        sample.Origin = Vec3{ 1.0f, 2.0f, 3.0f };
        sample.Direction = Vec3{ 1.0f, 0.0f, 0.0f };
        sample.MaxDistance = k_Length;
        return sample;
    }

    auto EngineSurface(float fraction, std::int16_t material, std::uint8_t flags) -> CollisionResult
    {
        CollisionResult result{};
        result.IsHit = true;
        result.Kind = ResultKind::Structure;
        result.Fraction = fraction;
        result.Material = material;
        result.SurfaceFlags = flags;
        return result;
    }

    auto EngineObject(float fraction, std::uint32_t handle) -> CollisionResult
    {
        CollisionResult result{};
        result.IsHit = true;
        result.Kind = ResultKind::Object;
        result.Fraction = fraction;
        result.ObjectHandle = handle;
        return result;
    }

    auto OursSurface(float distance, std::int16_t material, std::uint8_t flags) -> Hit
    {
        Hit hit{};
        hit.IsHit = true;
        hit.Distance = distance;
        hit.Material = material;
        hit.SurfaceFlags = flags;
        return hit;
    }

    auto TickSurface(float distance, std::int16_t material, std::uint8_t flags) -> RaycastHit
    {
        RaycastHit hit{};
        hit.Hit = true;
        hit.Kind = HitKind::Static;
        hit.Distance = distance;
        hit.Material = material;
        hit.SurfaceFlags = flags;
        return hit;
    }

    auto TickObject(float distance, std::uint32_t handle) -> RaycastHit
    {
        RaycastHit hit{};
        hit.Hit = true;
        hit.Kind = HitKind::Dynamic;
        hit.Distance = distance;
        hit.ObjectHandle = handle;
        return hit;
    }

    // A sample whose static comparison differs by the given distance.
    auto SampleWithDistanceDelta(float delta) -> OracleSample
    {
        OracleSample sample{ MakeSample() };
        sample.EngineStatic = EngineSurface(0.5f, 7, 0);
        sample.OursStatic = OursSurface((0.5f * k_Length) + delta, 7, 0);
        return sample;
    }
}

TEST(RaycastOracleCompare, ClassifyHitSeparatesNothingMapAndObject)
{
    EXPECT_EQ(ClassifyHit(CollisionResult{}), HitClass::None);
    EXPECT_EQ(ClassifyHit(EngineSurface(0.1f, 0, 0)), HitClass::Static);
    EXPECT_EQ(ClassifyHit(EngineObject(0.1f, 5)), HitClass::Dynamic);
}

TEST(RaycastOracleCompare, StaticMissOnBothSidesIsNotADifference)
{
    const RayDifference difference{ CompareStatic(MakeSample()) };

    EXPECT_TRUE(difference.IsCompared);
    EXPECT_FALSE(difference.IsAnyDifferent());
}

TEST(RaycastOracleCompare, StaticHitOnOneSideOnlyIsAHitDifference)
{
    OracleSample sample{ MakeSample() };
    sample.EngineStatic = EngineSurface(0.5f, 3, 0);

    const RayDifference difference{ CompareStatic(sample) };

    EXPECT_TRUE(difference.IsHitDifferent);
    EXPECT_TRUE(difference.IsEngineHit);
    EXPECT_FALSE(difference.IsOursHit);
    EXPECT_FALSE(difference.IsDistanceDifferent);
}

TEST(RaycastOracleCompare, StaticDistanceIsAFractionOfTheVector)
{
    const RayDifference same{ CompareStatic(SampleWithDistanceDelta(0.0f)) };

    EXPECT_FLOAT_EQ(same.EngineDistance, 0.5f * k_Length);
    EXPECT_FALSE(same.IsAnyDifferent());
}

TEST(RaycastOracleCompare, StaticDistanceToleranceIsTheLimit)
{
    EXPECT_FALSE(CompareStatic(SampleWithDistanceDelta(k_DistanceTolerance * 0.5f)).IsDistanceDifferent);
    EXPECT_FALSE(CompareStatic(SampleWithDistanceDelta(-k_DistanceTolerance * 0.5f)).IsDistanceDifferent);

    const RayDifference distant{ CompareStatic(SampleWithDistanceDelta(k_DistanceTolerance * 2.0f)) };
    EXPECT_TRUE(distant.IsDistanceDifferent);
    EXPECT_NEAR(distant.DistanceDelta, k_DistanceTolerance * 2.0f, 0.001f);
}

TEST(RaycastOracleCompare, StaticMaterialAndFlagsAreComparedWhenBothHit)
{
    OracleSample sample{ SampleWithDistanceDelta(0.0f) };
    sample.OursStatic.Material = 8;
    sample.OursStatic.SurfaceFlags = 0x02;

    const RayDifference difference{ CompareStatic(sample) };

    EXPECT_TRUE(difference.IsMaterialDifferent);
    EXPECT_TRUE(difference.IsFlagsDifferent);
    EXPECT_FALSE(difference.IsDistanceDifferent);
    EXPECT_FALSE(difference.IsHitDifferent);
}

TEST(RaycastOracleCompare, ObjectsAreNotComparedWithoutTheRayOfTheTick)
{
    OracleSample sample{ MakeSample() };
    sample.EngineObjects = EngineObject(0.2f, 9);

    EXPECT_FALSE(CompareObjects(sample).IsCompared);
}

TEST(RaycastOracleCompare, ObjectHandleOfTheSameKindMustMatch)
{
    OracleSample sample{ MakeSample() };
    sample.EngineObjects = EngineObject(0.2f, 9);
    sample.OursTick = TickObject(0.2f * k_Length, 9);

    EXPECT_FALSE(CompareObjects(sample).IsAnyDifferent());

    sample.OursTick = TickObject(0.2f * k_Length, 10);
    const RayDifference other{ CompareObjects(sample) };

    EXPECT_TRUE(other.IsObjectDifferent);
    EXPECT_FALSE(other.IsKindDifferent);
}

TEST(RaycastOracleCompare, SurfaceAgainstObjectIsAKindDifference)
{
    OracleSample sample{ MakeSample() };
    sample.EngineObjects = EngineSurface(0.2f, 1, 0);
    sample.OursTick = TickObject(0.2f * k_Length, 9);

    const RayDifference difference{ CompareObjects(sample) };

    EXPECT_TRUE(difference.IsKindDifferent);
    EXPECT_FALSE(difference.IsObjectDifferent);
    EXPECT_FALSE(difference.IsMaterialDifferent);
}

TEST(RaycastOracleCompare, ObjectsSideComparesMaterialAndFlagsOfASurface)
{
    OracleSample sample{ MakeSample() };
    sample.EngineObjects = EngineSurface(0.4f, 4, 0x01);
    sample.OursTick = TickSurface(0.4f * k_Length, 4, 0x01);

    EXPECT_FALSE(CompareObjects(sample).IsAnyDifferent());

    sample.OursTick = TickSurface(0.4f * k_Length, 5, 0x00);
    const RayDifference difference{ CompareObjects(sample) };

    EXPECT_TRUE(difference.IsMaterialDifferent);
    EXPECT_TRUE(difference.IsFlagsDifferent);
}

TEST(RaycastOracleCompare, CompareFillsBothSides)
{
    OracleSample sample{ SampleWithDistanceDelta(1.0f) };
    sample.EngineObjects = EngineObject(0.2f, 9);
    sample.OursTick = TickObject(0.2f * k_Length, 9);

    const OracleComparison comparison{ Compare(sample) };

    EXPECT_TRUE(comparison.Static.IsDistanceDifferent);
    EXPECT_TRUE(comparison.Objects.IsCompared);
    EXPECT_FALSE(comparison.Objects.IsAnyDifferent());
}

TEST(RaycastOracleCompare, RootObjectFollowsTheParentChain)
{
    ObjectTable objects{};
    objects[0x10].ParentHandle = k_NoHandle;
    objects[0x20].ParentHandle = 0x10;
    objects[0x30].ParentHandle = 0x20;

    EXPECT_EQ(ResolveRootObject(objects, 0x30), 0x10u);
    EXPECT_EQ(ResolveRootObject(objects, 0x10), 0x10u);
    EXPECT_EQ(ResolveRootObject(objects, 0x99), 0x99u);
}

TEST(RaycastOracleCompare, RootObjectStopsOnACycle)
{
    ObjectTable objects{};
    objects[1].ParentHandle = 2;
    objects[2].ParentHandle = 1;

    const std::uint32_t root{ ResolveRootObject(objects, 1) };

    EXPECT_TRUE(root == 1u || root == 2u);
}

TEST(RaycastOracleCompare, RandomDirectionsAreUnitAndRepeatForTheSameSeed)
{
    std::mt19937 first{ 7 };
    std::mt19937 second{ 7 };

    for (int index{ 0 }; index < 64; ++index)
    {
        const Vec3 direction{ RandomDirection(first) };
        const float length{ std::sqrt((direction.X * direction.X) + (direction.Y * direction.Y) + (direction.Z * direction.Z)) };

        EXPECT_NEAR(length, 1.0f, 1e-4f);

        const Vec3 again{ RandomDirection(second) };
        EXPECT_EQ(direction.X, again.X);
        EXPECT_EQ(direction.Y, again.Y);
        EXPECT_EQ(direction.Z, again.Z);
    }
}

TEST(RaycastOracleRecord, CountsRaysHitsAndDifferences)
{
    OracleStats stats{};

    OracleSample matching{ SampleWithDistanceDelta(0.0f) };
    Record(stats, matching, Compare(matching));

    OracleSample missed{ MakeSample() };
    missed.Source = RaySource::Random;
    missed.EngineStatic = EngineSurface(0.3f, 1, 0);
    Record(stats, missed, Compare(missed));

    EXPECT_EQ(stats.TickRays, 1u);
    EXPECT_EQ(stats.RandomRays, 1u);
    EXPECT_EQ(stats.Static.Rays, 2u);
    EXPECT_EQ(stats.Static.EngineHits, 2u);
    EXPECT_EQ(stats.Static.EngineStructureHits, 2u);
    EXPECT_EQ(stats.Static.OursHits, 1u);
    EXPECT_EQ(stats.Static.HitMismatches, 1u);
    EXPECT_EQ(stats.Static.BothHit, 1u);
    EXPECT_EQ(stats.Objects.Rays, 0u);
    ASSERT_EQ(stats.Worst.size(), 1u);
    EXPECT_TRUE(stats.Worst[0].Difference.IsHitDifferent);
}

TEST(RaycastOracleRecord, ARandomRayThatHitAnObjectIsCountedApart)
{
    OracleStats stats{};

    OracleSample sample{ MakeSample() };
    sample.Source = RaySource::Random;
    sample.EngineObjects = EngineObject(0.1f, 3);
    Record(stats, sample, Compare(sample));

    EXPECT_EQ(stats.RandomObjectHits, 1u);
    EXPECT_EQ(stats.Objects.Rays, 0u);
}

TEST(RaycastOracleRecord, WorstListKeepsTheWorstSortedAndCapped)
{
    OracleStats stats{};

    for (std::size_t index{ 0 }; index < k_WorstCount + 10; ++index)
    {
        OracleSample sample{ SampleWithDistanceDelta(0.1f + (0.1f * static_cast<float>(index))) };
        Record(stats, sample, Compare(sample));
    }

    ASSERT_EQ(stats.Worst.size(), k_WorstCount);

    for (std::size_t index{ 1 }; index < stats.Worst.size(); ++index)
    {
        EXPECT_GE(stats.Worst[index - 1].Score, stats.Worst[index].Score);
    }

    // The smallest delta of the kept ones is the 11th of the 30 fabricated (0.1 + 0.1 * 10).
    EXPECT_NEAR(stats.Worst.back().Difference.DistanceDelta, 1.1f, 0.01f);
    EXPECT_NEAR(stats.Worst.front().Difference.DistanceDelta, 3.0f, 0.01f);
}

TEST(RaycastOracleRecord, AHitDifferenceOutranksADistanceDifference)
{
    OracleStats stats{};

    OracleSample distant{ SampleWithDistanceDelta(5.0f) };
    Record(stats, distant, Compare(distant));

    OracleSample missed{ MakeSample() };
    missed.EngineStatic = EngineSurface(0.3f, 1, 0);
    Record(stats, missed, Compare(missed));

    ASSERT_EQ(stats.Worst.size(), 2u);
    EXPECT_TRUE(stats.Worst[0].Difference.IsHitDifferent);
    EXPECT_TRUE(stats.Worst[1].Difference.IsDistanceDifferent);
}

TEST(RaycastOracleRecord, LagIsCountedPerOutcome)
{
    OracleStats stats{};
    RecordLag(stats, 1);
    RecordLag(stats, 3);

    EXPECT_EQ(stats.Outcomes, 2u);
    EXPECT_EQ(stats.LagMax, 3u);
    EXPECT_DOUBLE_EQ(Summarize(stats).LagMean, 2.0);
}

TEST(RaycastOracleCost, PercentilesOfAKnownDistribution)
{
    OracleStats stats{};

    // 99 calls of 4 us and one of 100 us.
    for (int index{ 0 }; index < 99; ++index)
    {
        OracleSample sample{ MakeSample() };
        sample.EngineStaticNanoseconds = 4000;
        sample.EngineObjectsNanoseconds = 8000;
        Record(stats, sample, Compare(sample));
    }

    OracleSample slow{ MakeSample() };
    slow.EngineStaticNanoseconds = 100000;
    slow.EngineObjectsNanoseconds = 8000;
    Record(stats, slow, Compare(slow));

    const OracleSummary summary{ Summarize(stats) };

    EXPECT_NEAR(summary.StaticMean, (99.0 * 4.0 + 100.0) / 100.0, 1e-6);
    EXPECT_NEAR(summary.StaticP50, 4.0, 0.3);
    EXPECT_NEAR(summary.StaticP99, 4.0, 0.3);
    EXPECT_NEAR(summary.ObjectsMean, 8.0, 1e-6);
    EXPECT_NEAR(summary.ObjectsP50, 8.0, 0.3);
    EXPECT_FLOAT_EQ(stats.StaticCost.MaxMicroseconds, 100.0f);
}

TEST(RaycastOracleCost, TheLastBucketTakesTheTail)
{
    CostHistogram histogram{};
    OracleStats stats{};

    OracleSample slow{ MakeSample() };
    slow.EngineStaticNanoseconds = 5'000'000;
    Record(stats, slow, Compare(slow));

    EXPECT_DOUBLE_EQ(Percentile(stats.StaticCost, 0.99), 5000.0);
    EXPECT_DOUBLE_EQ(Percentile(histogram, 0.5), 0.0);
}

TEST(RaycastOracleReport, SummaryAndReportCarryTheNumbers)
{
    OracleStats stats{};

    OracleSample sample{ SampleWithDistanceDelta(2.0f) };
    sample.EngineStaticNanoseconds = 3000;
    sample.EngineObjectsNanoseconds = 6000;
    Record(stats, sample, Compare(sample));
    RecordLag(stats, 1);

    const std::string summary{ FormatSummary(stats) };
    EXPECT_NE(summary.find("rays=1"), std::string::npos);
    EXPECT_NE(summary.find("dist_diff=1"), std::string::npos);
    EXPECT_NE(summary.find("engine us static mean=3.00"), std::string::npos);
    EXPECT_NE(summary.find("lag ticks mean=1.00"), std::string::npos);

    const std::string report{ FormatReport(stats, "header line") };
    EXPECT_EQ(report.rfind("header line", 0), 0u);
    EXPECT_NE(report.find("Worst 1 differences"), std::string::npos);
    EXPECT_NE(report.find("origin=(1.000, 2.000, 3.000)"), std::string::npos);
    EXPECT_NE(report.find("distance(2.000 m)"), std::string::npos);
}

TEST(RaycastOracleReport, SkippedRaysAreReportedApart)
{
    OracleStats stats{};
    stats.SkippedRays = 3;

    EXPECT_NE(FormatSummary(stats).find("skipped_rays=3"), std::string::npos);
    EXPECT_NE(FormatReport(stats, "h").find("rays skipped (not valid) 3"), std::string::npos);
}

TEST(RaycastOracleReport, EmptyStatsFormatWithoutDividingByZero)
{
    const OracleStats stats{};

    EXPECT_NE(FormatSummary(stats).find("rays=0"), std::string::npos);
    EXPECT_NE(FormatReport(stats, "h").find("Worst 0 differences"), std::string::npos);
}