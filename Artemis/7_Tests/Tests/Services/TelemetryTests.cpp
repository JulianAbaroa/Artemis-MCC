#include <gtest/gtest.h>

import Services.Telemetry.State;
import Services.Telemetry.System;
import Services.Telemetry.Type;
import std;

// Services.Telemetry: the cumulative counters, the sweep histogram, the percentiles of a difference of two photos and the stage times.

namespace
{
    using Services::Telemetry::State::TelemetryStore;
    using Services::Telemetry::System::HistogramPercentileNs;
    using Services::Telemetry::System::OtherStageNs;
    using Services::Telemetry::System::SweepBucket;
    using Services::Telemetry::System::Subtract;
    using Services::Telemetry::System::TelemetryService;
    using Services::Telemetry::Type::PerfSnapshot;
    using Services::Telemetry::Type::k_SweepBucketCount;
    using Services::Telemetry::Type::k_SweepHistogramSize;

    constexpr std::array<std::uint64_t, 0> k_NoStages{};

    auto Photo(TelemetryStore& store) -> PerfSnapshot
    {
        PerfSnapshot snapshot{};
        store.CopyPerf(snapshot);
        return snapshot;
    }
}

TEST(TelemetryTests, CumulativeCountersSurviveUpdate)
{
    TelemetryStore store{};
    TelemetryService service{ store };

    store.RecordSimTick(3, 9'000);
    store.RecordReadWait(500, false);
    store.RecordReadWait(700, true);
    store.RecordSweep(2'000, k_NoStages);

    std::this_thread::sleep_for(std::chrono::milliseconds{ 2 });
    service.Update();

    // The window counters went back to zero, the cumulative ones did not.
    EXPECT_EQ(store.m_SimNs.load(), 0u);
    EXPECT_EQ(store.m_Sweeps.load(), 0u);
    EXPECT_EQ(store.m_ReadTimeouts.load(), 0u);

    const PerfSnapshot photo{ Photo(store) };
    EXPECT_EQ(photo.Ticks, 3u);
    EXPECT_EQ(photo.EngineNs, 9'000u);
    EXPECT_EQ(photo.ReadWaitNs, 1'200u);
    EXPECT_EQ(photo.ReadTimeouts, 1u);
    EXPECT_EQ(photo.Sweeps, 1u);
    EXPECT_EQ(photo.SweepNs, 2'000u);

    store.RecordSimTick(-5, 100);
    EXPECT_EQ(Photo(store).Ticks, 3u);
    EXPECT_EQ(Photo(store).EngineNs, 9'100u);
}

TEST(TelemetryTests, SweepFallsInTheBucketOfItsDuration)
{
    EXPECT_EQ(SweepBucket(0), 0u);
    EXPECT_EQ(SweepBucket(24'900), 0u);
    EXPECT_EQ(SweepBucket(25'000), 1u);
    EXPECT_EQ(SweepBucket(6'399'999), k_SweepBucketCount - 1);
    EXPECT_EQ(SweepBucket(6'400'000), k_SweepBucketCount);
    EXPECT_EQ(SweepBucket(900'000'000), k_SweepBucketCount);

    TelemetryStore store{};
    for (std::uint64_t ns : { 0ull, 24'900ull, 25'000ull, 6'399'999ull, 6'400'000ull, 50'000'000ull }) store.RecordSweep(ns, k_NoStages);

    const PerfSnapshot photo{ Photo(store) };
    EXPECT_EQ(photo.SweepHistogram.size(), k_SweepHistogramSize);
    EXPECT_EQ(photo.SweepHistogram[0], 2u);
    EXPECT_EQ(photo.SweepHistogram[1], 1u);
    EXPECT_EQ(photo.SweepHistogram[k_SweepBucketCount - 1], 1u);
    EXPECT_EQ(photo.SweepHistogram[k_SweepBucketCount], 2u);
    EXPECT_EQ(std::accumulate(photo.SweepHistogram.begin(), photo.SweepHistogram.end(), std::uint64_t{ 0 }), photo.Sweeps);
}

TEST(TelemetryTests, PercentilesOfTheDifferenceOfTwoPhotos)
{
    TelemetryStore store{};

    // Before the period: a long sweep that the difference must not count.
    store.RecordSweep(3'000'000, k_NoStages);
    const PerfSnapshot before{ Photo(store) };

    // 90 sweeps of 100 us (bucket 4), 9 of 1 ms (bucket 40) and one of 5 ms (bucket 200).
    for (int i{ 0 }; i < 90; ++i) store.RecordSweep(100'000, k_NoStages);
    for (int i{ 0 }; i < 9; ++i) store.RecordSweep(1'000'000, k_NoStages);
    store.RecordSweep(5'000'000, k_NoStages);

    const PerfSnapshot delta{ Subtract(Photo(store), before) };
    EXPECT_EQ(delta.Sweeps, 100u);
    EXPECT_EQ(delta.SweepMaxNs, 5'000'000u);
    EXPECT_EQ(HistogramPercentileNs(delta.SweepHistogram, 0.5, delta.SweepMaxNs), 125'000u);
    EXPECT_EQ(HistogramPercentileNs(delta.SweepHistogram, 0.9, delta.SweepMaxNs), 125'000u);
    EXPECT_EQ(HistogramPercentileNs(delta.SweepHistogram, 0.99, delta.SweepMaxNs), 1'025'000u);
    // The top rank is capped by the real maximum, not by the upper edge of its bucket.
    EXPECT_EQ(HistogramPercentileNs(delta.SweepHistogram, 1.0, delta.SweepMaxNs), 5'000'000u);
    EXPECT_EQ(HistogramPercentileNs(PerfSnapshot{}.SweepHistogram, 0.5, 0), 0u);
}

TEST(TelemetryTests, OverflowBucketPercentileIsTheMaximum)
{
    TelemetryStore store{};
    store.RecordSweep(10'000, k_NoStages);
    store.RecordSweep(20'000'000, k_NoStages);

    const PerfSnapshot photo{ Photo(store) };
    EXPECT_EQ(HistogramPercentileNs(photo.SweepHistogram, 0.99, photo.SweepMaxNs), 20'000'000u);
}

TEST(TelemetryTests, MaximumRestartsWithEachPhoto)
{
    TelemetryStore store{};
    store.RecordSweep(4'000, k_NoStages);
    EXPECT_EQ(Photo(store).SweepMaxNs, 4'000u);
    EXPECT_EQ(Photo(store).SweepMaxNs, 0u);

    store.RecordSweep(1'000, k_NoStages);
    EXPECT_EQ(Photo(store).SweepMaxNs, 1'000u);
}

TEST(TelemetryTests, StageTimesAddUpByNameAndExtraStagesAreIgnored)
{
    TelemetryStore store{};
    const std::array<std::string_view, 3> names{ "Capture", "Tick", "Other" };
    store.SetStageNames(names);

    const std::array<std::uint64_t, 3> first{ 10, 20, 5 };
    const std::array<std::uint64_t, 3> second{ 1, 2, 3 };
    store.RecordSweep(40, first);
    store.RecordSweep(10, second);

    ASSERT_EQ(store.GetStageNames().size(), 3u);
    EXPECT_EQ(store.GetStageNames()[1], "Tick");

    const PerfSnapshot photo{ Photo(store) };
    EXPECT_EQ(photo.StageNs[0], 11u);
    EXPECT_EQ(photo.StageNs[1], 22u);
    EXPECT_EQ(photo.StageNs[2], 8u);
    EXPECT_EQ(photo.StageNs[3], 0u);

    const std::vector<std::uint64_t> tooMany(100, 1);
    store.RecordSweep(100, tooMany);
    EXPECT_EQ(Photo(store).StageNs[31], 1u);
}

TEST(TelemetryTests, OtherIsTheSweepMinusItsStagesAndNeverNegative)
{
    const std::array<std::uint64_t, 2> stages{ 30, 20 };
    EXPECT_EQ(OtherStageNs(100, stages), 50u);
    EXPECT_EQ(OtherStageNs(50, stages), 0u);
    EXPECT_EQ(OtherStageNs(10, stages), 0u);
    EXPECT_EQ(OtherStageNs(70, k_NoStages), 70u);
}

TEST(TelemetryTests, SubtractClampsACounterThatWentBackwards)
{
    PerfSnapshot earlier{};
    PerfSnapshot later{};
    earlier.Ticks = 10;
    later.Ticks = 4;
    later.EngineNs = 7;

    const PerfSnapshot delta{ Subtract(later, earlier) };
    EXPECT_EQ(delta.Ticks, 0u);
    EXPECT_EQ(delta.EngineNs, 7u);
}