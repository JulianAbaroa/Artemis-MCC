#include <gtest/gtest.h>

import Common.State.Type;
import std;

namespace
{
    using Common::State::Type::ByteBoundedQueue;
    using std::chrono::milliseconds;
}

TEST(ByteBoundedQueueTests, ClosedQueueDropsTheEntry)
{
    ByteBoundedQueue<int> queue{ 100 };
    const auto result{ queue.Push(1, 10) };
    EXPECT_FALSE(result.IsQueued);
    EXPECT_EQ(queue.Size(), 0u);
}

TEST(ByteBoundedQueueTests, KeepsOrderAndCountsBytes)
{
    ByteBoundedQueue<int> queue{ 100 };
    queue.SetOpen(true);
    EXPECT_TRUE(queue.Push(1, 30).IsQueued);
    EXPECT_TRUE(queue.Push(2, 30).IsQueued);
    EXPECT_EQ(queue.Size(), 2u);
    EXPECT_EQ(queue.Bytes(), 60u);
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), 1);
    EXPECT_EQ(queue.Bytes(), 30u);
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), 2);
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), std::nullopt);
    EXPECT_EQ(queue.Bytes(), 0u);
}

TEST(ByteBoundedQueueTests, FullByBytesWithFewEntriesMakesTheProducerWaitAndNothingIsLost)
{
    ByteBoundedQueue<int> queue{ 100 };
    queue.SetOpen(true);
    EXPECT_TRUE(queue.Push(1, 60).IsQueued);

    std::atomic<bool> isPushed{};
    ByteBoundedQueue<int>::PushResult result{};
    std::thread producer{ [&] {
        result = queue.Push(2, 60);
        isPushed.store(true);
    } };

    std::this_thread::sleep_for(milliseconds{ 100 });
    EXPECT_FALSE(isPushed.load());
    EXPECT_EQ(queue.Size(), 1u);

    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), 1);
    producer.join();
    EXPECT_TRUE(isPushed.load());
    EXPECT_TRUE(result.IsQueued);
    EXPECT_TRUE(result.IsWaited);
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), 2);
}

TEST(ByteBoundedQueueTests, AnEntryLargerThanTheCapStillGoesInWhenTheQueueIsEmpty)
{
    ByteBoundedQueue<int> queue{ 100 };
    queue.SetOpen(true);
    const auto result{ queue.Push(1, 500) };
    EXPECT_TRUE(result.IsQueued);
    EXPECT_FALSE(result.IsWaited);
    EXPECT_EQ(queue.Bytes(), 500u);
}

TEST(ByteBoundedQueueTests, ClosingWakesTheWaitingProducerAndDropsItsEntry)
{
    ByteBoundedQueue<int> queue{ 100 };
    queue.SetOpen(true);
    EXPECT_TRUE(queue.Push(1, 80).IsQueued);

    ByteBoundedQueue<int>::PushResult result{};
    std::thread producer{ [&] { result = queue.Push(2, 80); } };
    std::this_thread::sleep_for(milliseconds{ 50 });
    queue.SetOpen(false);
    producer.join();

    EXPECT_FALSE(result.IsQueued);
    EXPECT_TRUE(result.IsWaited);
    // The closed queue still drains.
    EXPECT_EQ(queue.WaitPop(milliseconds{ 0 }), 1);
}

TEST(ByteBoundedQueueTests, ClearEmptiesTheQueueAndTheBytes)
{
    ByteBoundedQueue<int> queue{ 100 };
    queue.SetOpen(true);
    queue.Push(1, 40);
    queue.Push(2, 40);
    queue.Clear();
    EXPECT_EQ(queue.Size(), 0u);
    EXPECT_EQ(queue.Bytes(), 0u);
    EXPECT_TRUE(queue.Push(3, 100).IsQueued);
}