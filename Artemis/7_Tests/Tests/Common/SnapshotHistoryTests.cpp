#include <gtest/gtest.h>

import Common.State.Type;
import std;

namespace
{
    using Common::State::Type::IsConsecutiveByKey;
    using Common::State::Type::PickByKey;
    using Common::State::Type::PickKind;
    using Common::State::Type::SnapshotHistory;

    using Keys = std::vector<std::optional<std::uint32_t>>;

    struct Frame
    {
        std::optional<std::uint32_t> Tick{};
        int Payload{};
    };

    struct FrameTickOf
    {
        auto operator()(const Frame& frame) const -> std::optional<std::uint32_t> { return frame.Tick; }
    };

    using History = SnapshotHistory<Frame, 4, FrameTickOf>;
}

TEST(SnapshotHistoryTests, PickExactWhenTheTargetIsThere)
{
    const Keys keys{ 10, 11, 12, 13 };

    const auto result = PickByKey(keys, 12);
    EXPECT_EQ(result.Kind, PickKind::Exact);
    EXPECT_EQ(result.Index, 2u);
}

TEST(SnapshotHistoryTests, PickTheNearestOlderWhenTheTargetIsMissing)
{
    const Keys keys{ 10, 11, 13, 14 };

    const auto result = PickByKey(keys, 12);
    EXPECT_EQ(result.Kind, PickKind::Older);
    EXPECT_EQ(result.Index, 1u);
}

TEST(SnapshotHistoryTests, PickTheNewestWhenNothingIsAtOrBeforeTheTarget)
{
    const Keys keys{ 20, 21, 22 };

    const auto result = PickByKey(keys, 12);
    EXPECT_EQ(result.Kind, PickKind::Newer);
    EXPECT_EQ(result.Index, 2u);
}

TEST(SnapshotHistoryTests, PickIgnoresEntriesWithoutKeyAndNothingKeyedIsNone)
{
    const Keys withGap{ std::nullopt, 10, std::nullopt };
    EXPECT_EQ(PickByKey(withGap, 10).Index, 1u);

    const Keys none{ std::nullopt, std::nullopt };
    EXPECT_EQ(PickByKey(none, 10).Kind, PickKind::None);
    EXPECT_EQ(PickByKey(Keys{}, 10).Kind, PickKind::None);
}

TEST(SnapshotHistoryTests, ConsecutiveNeedsTheKeyBeforeTheNewest)
{
    EXPECT_TRUE(IsConsecutiveByKey(Keys{ 10, 11, 12 }));
    // Sparse: only decision ticks arrive.
    EXPECT_FALSE(IsConsecutiveByKey(Keys{ 10, 15, 20 }));
    EXPECT_FALSE(IsConsecutiveByKey(Keys{ 10 }));
    EXPECT_FALSE(IsConsecutiveByKey(Keys{}));
}

TEST(SnapshotHistoryTests, KeepsTheLastCapacityAndAcquireIsTheNewest)
{
    History history{};
    EXPECT_EQ(history.Acquire(), nullptr);

    for (std::uint32_t tick{ 1 }; tick <= 6; ++tick) history.Publish(Frame{ .Tick = tick, .Payload = static_cast<int>(tick) });

    ASSERT_NE(history.Acquire(), nullptr);
    EXPECT_EQ(history.Acquire()->Payload, 6);
    EXPECT_EQ(history.FindKey(2), nullptr);
    ASSERT_NE(history.FindKey(3), nullptr);
    EXPECT_EQ(history.FindKey(3)->Payload, 3);
    EXPECT_TRUE(history.IsConsecutive());
}

TEST(SnapshotHistoryTests, PickForReturnsTheSceneOfTheTarget)
{
    History history{};
    for (std::uint32_t tick{ 10 }; tick <= 13; ++tick) history.Publish(Frame{ .Tick = tick, .Payload = static_cast<int>(tick) });

    const auto exact = history.PickFor(12);
    ASSERT_NE(exact.Snapshot, nullptr);
    EXPECT_EQ(exact.Snapshot->Payload, 12);
    EXPECT_EQ(exact.Kind, PickKind::Exact);

    const auto newer = history.PickFor(5);
    EXPECT_EQ(newer.Snapshot->Payload, 13);
    EXPECT_EQ(newer.Kind, PickKind::Newer);

    const auto older = history.PickFor(99);
    EXPECT_EQ(older.Snapshot->Payload, 13);
    EXPECT_EQ(older.Kind, PickKind::Older);
}

TEST(SnapshotHistoryTests, PickForWithoutKeysGivesTheNewestAndNone)
{
    History history{};
    history.Publish(Frame{ .Tick = std::nullopt, .Payload = 7 });

    const auto pick = history.PickFor(5);
    ASSERT_NE(pick.Snapshot, nullptr);
    EXPECT_EQ(pick.Snapshot->Payload, 7);
    EXPECT_EQ(pick.Kind, PickKind::None);
}

TEST(SnapshotHistoryTests, WaitForKeyReturnsAtOnceWhenItIsThereAndTimesOutWhenNot)
{
    History history{};
    history.Publish(Frame{ .Tick = 10, .Payload = 1 });

    EXPECT_TRUE(history.WaitForKey(10, std::chrono::microseconds{ 1000 }));

    const auto start = std::chrono::steady_clock::now();
    EXPECT_FALSE(history.WaitForKey(11, std::chrono::milliseconds{ 20 }));
    EXPECT_GE(std::chrono::steady_clock::now() - start, std::chrono::milliseconds{ 15 });
}

TEST(SnapshotHistoryTests, WaitForKeyWakesWhenAnotherThreadPublishes)
{
    History history{};
    history.Publish(Frame{ .Tick = 10, .Payload = 1 });

    std::thread writer{ [&history] {
        std::this_thread::sleep_for(std::chrono::milliseconds{ 20 });
        history.Publish(Frame{ .Tick = 11, .Payload = 2 });
    } };

    const auto start = std::chrono::steady_clock::now();
    const bool isReached{ history.WaitForKey(11, std::chrono::seconds{ 5 }) };
    const auto waited = std::chrono::steady_clock::now() - start;
    writer.join();

    EXPECT_TRUE(isReached);
    EXPECT_LT(waited, std::chrono::seconds{ 2 });
}

TEST(SnapshotHistoryTests, CleanupEmptiesAndWakesTheWaiter)
{
    History history{};
    history.Publish(Frame{ .Tick = 10, .Payload = 1 });

    std::thread cleaner{ [&history] {
        std::this_thread::sleep_for(std::chrono::milliseconds{ 20 });
        history.Cleanup();
    } };

    const auto start = std::chrono::steady_clock::now();
    const bool isReached{ history.WaitForKey(50, std::chrono::seconds{ 5 }) };
    const auto waited = std::chrono::steady_clock::now() - start;
    cleaner.join();

    EXPECT_FALSE(isReached);
    EXPECT_LT(waited, std::chrono::seconds{ 2 });
    EXPECT_EQ(history.Acquire(), nullptr);
}