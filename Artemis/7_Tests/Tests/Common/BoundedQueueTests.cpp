#include <gtest/gtest.h>

import Common.State.Type;
import std;

namespace
{
    using Common::State::Type::BoundedQueue;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
}

TEST(BoundedQueueTests, ClosedQueueDoesNotQueue)
{
    BoundedQueue<int> queue{ 4 };
    EXPECT_FALSE(queue.IsOpen());

    const auto result{ queue.Push(1, milliseconds{ 0 }) };
    EXPECT_FALSE(result.IsQueued);
    EXPECT_FALSE(result.IsTimedOut);
    EXPECT_EQ(queue.Size(), 0u);
}

TEST(BoundedQueueTests, KeepsTheOrderAndReportsTheDepth)
{
    BoundedQueue<int> queue{ 4 };
    queue.SetOpen(true);

    for (int value{ 1 }; value <= 4; ++value)
    {
        const auto result{ queue.Push(value, milliseconds{ 0 }) };
        EXPECT_TRUE(result.IsQueued);
        EXPECT_FALSE(result.IsWaited);
        EXPECT_EQ(result.Depth, static_cast<std::size_t>(value));
    }

    for (int value{ 1 }; value <= 4; ++value) EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), value);
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), std::nullopt);
}

TEST(BoundedQueueTests, FullQueueMakesTheProducerWaitUpToTheTimeoutAndThenGivesTheEntryUp)
{
    BoundedQueue<int> queue{ 2 };
    queue.SetOpen(true);
    EXPECT_TRUE(queue.Push(1, milliseconds{ 0 }).IsQueued);
    EXPECT_TRUE(queue.Push(2, milliseconds{ 0 }).IsQueued);

    const auto start{ steady_clock::now() };
    const auto result{ queue.Push(3, milliseconds{ 60 }) };
    const auto elapsed{ std::chrono::duration<double, std::milli>(steady_clock::now() - start).count() };

    EXPECT_FALSE(result.IsQueued);
    EXPECT_TRUE(result.IsTimedOut);
    EXPECT_TRUE(result.IsWaited);
    EXPECT_GE(elapsed, 50.0);
    EXPECT_LT(elapsed, 500.0);

    // Nothing of the lost entry is in the queue and the others keep their order.
    EXPECT_EQ(queue.Size(), 2u);
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), 1);
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), 2);
}

TEST(BoundedQueueTests, ProducerThatWaitsGetsInAsSoonAsTheConsumerTakesOne)
{
    BoundedQueue<int> queue{ 1 };
    queue.SetOpen(true);
    EXPECT_TRUE(queue.Push(1, milliseconds{ 0 }).IsQueued);

    BoundedQueue<int>::PushResult result{};
    std::jthread producer{ [&] { result = queue.Push(2, milliseconds{ 5000 }); } };

    std::this_thread::sleep_for(milliseconds{ 50 });
    EXPECT_EQ(queue.Size(), 1u);
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), 1);
    producer.join();

    EXPECT_TRUE(result.IsQueued);
    EXPECT_TRUE(result.IsWaited);
    EXPECT_FALSE(result.IsTimedOut);
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), 2);
}

TEST(BoundedQueueTests, ClosingWakesAProducerThatWaitsAndItsEntryIsNotQueued)
{
    BoundedQueue<int> queue{ 1 };
    queue.SetOpen(true);
    EXPECT_TRUE(queue.Push(1, milliseconds{ 0 }).IsQueued);

    BoundedQueue<int>::PushResult result{};
    const auto start{ steady_clock::now() };
    std::jthread producer{ [&] { result = queue.Push(2, milliseconds{ 5000 }); } };

    std::this_thread::sleep_for(milliseconds{ 30 });
    queue.SetOpen(false);
    producer.join();

    const double elapsed{ std::chrono::duration<double, std::milli>(steady_clock::now() - start).count() };
    EXPECT_LT(elapsed, 1000.0);
    EXPECT_FALSE(result.IsQueued);
    EXPECT_FALSE(result.IsTimedOut);
    EXPECT_EQ(queue.Size(), 1u);

    // A closed queue still drains.
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), 1);
}

TEST(BoundedQueueTests, WaitPopWaitsForAnEntryAndTimesOutWithoutOne)
{
    BoundedQueue<int> queue{ 4 };
    queue.SetOpen(true);
    EXPECT_EQ(queue.WaitPop(milliseconds{ 20 }), std::nullopt);

    std::jthread producer{ [&] {
        std::this_thread::sleep_for(milliseconds{ 30 });
        queue.Push(7, milliseconds{ 0 });
    } };
    EXPECT_EQ(queue.WaitPop(milliseconds{ 5000 }), 7);
}

TEST(BoundedQueueTests, ClearRemovesEverythingAndFreesTheProducer)
{
    BoundedQueue<std::shared_ptr<int>> queue{ 2 };
    queue.SetOpen(true);
    const auto value{ std::make_shared<int>(5) };
    EXPECT_TRUE(queue.Push(value, milliseconds{ 0 }).IsQueued);
    EXPECT_TRUE(queue.Push(value, milliseconds{ 0 }).IsQueued);
    EXPECT_EQ(value.use_count(), 3);

    EXPECT_EQ(queue.Clear(), 2u);
    EXPECT_EQ(queue.Size(), 0u);
    EXPECT_EQ(value.use_count(), 1);
    EXPECT_EQ(queue.Clear(), 0u);
    EXPECT_TRUE(queue.Push(value, milliseconds{ 0 }).IsQueued);
}

TEST(BoundedQueueTests, OneProducerAndOneConsumerLoseNothingAndKeepTheOrder)
{
    BoundedQueue<int> queue{ 4 };
    queue.SetOpen(true);

    constexpr int k_Count{ 2000 };
    std::vector<int> seen{};
    std::jthread consumer{ [&] {
        while (static_cast<int>(seen.size()) < k_Count)
        {
            const std::optional<int> value{ queue.WaitPop(milliseconds{ 100 }) };
            if (value.has_value()) seen.push_back(*value);
        }
    } };

    for (int value{ 0 }; value < k_Count; ++value) EXPECT_TRUE(queue.Push(value, milliseconds{ 5000 }).IsQueued);
    consumer.join();

    ASSERT_EQ(static_cast<int>(seen.size()), k_Count);
    for (int index{ 0 }; index < k_Count; ++index) ASSERT_EQ(seen[static_cast<std::size_t>(index)], index);
}
