#include <gtest/gtest.h>

import Common.State.Type;
import std;

namespace
{
    using Common::State::Type::Snapshot;
}

TEST(SnapshotTests, NothingPublishedIsNull)
{
    Snapshot<int> snapshot{};
    EXPECT_EQ(snapshot.Acquire(), nullptr);
}

TEST(SnapshotTests, AcquireReturnsTheLastPublished)
{
    Snapshot<std::string> snapshot{};
    snapshot.Publish("first");
    snapshot.Publish("second");

    const auto value = snapshot.Acquire();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "second");
}

TEST(SnapshotTests, CleanupKeepsWhatReadersAlreadyHold)
{
    Snapshot<int> snapshot{};
    snapshot.Publish(7);
    const auto held = snapshot.Acquire();

    snapshot.Cleanup();

    EXPECT_EQ(snapshot.Acquire(), nullptr);
    ASSERT_NE(held, nullptr);
    EXPECT_EQ(*held, 7);
}

// One writer and several readers: a reader never sees a value go back (the writer publishes in increasing order).
TEST(SnapshotTests, ReadersNeverSeeAnOlderValueAfterANewerOne)
{
    constexpr int k_Last{ 20000 };
    Snapshot<int> snapshot{};
    snapshot.Publish(0);
    std::atomic<bool> isBackwards{ false };

    std::vector<std::jthread> readers{};
    for (int i = 0; i < 3; ++i)
    {
        readers.emplace_back([&snapshot, &isBackwards] {
            int previous{ 0 };
            while (previous < k_Last)
            {
                const int current{ *snapshot.Acquire() };
                if (current < previous) isBackwards = true;
                previous = current;
            }
        });
    }
    for (int value = 1; value <= k_Last; ++value) snapshot.Publish(value);
    readers.clear();

    EXPECT_FALSE(isBackwards);
}