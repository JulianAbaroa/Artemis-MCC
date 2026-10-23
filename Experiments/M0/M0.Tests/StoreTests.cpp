#include <gtest/gtest.h>
#include <gmock/gmock.h>

import Common.State.Type;
import std;

namespace
{
    using Common::State::Type::Snapshot;
}

TEST(SnapshotTests, PublishThenAcquireReturnsTheValue)
{
    Snapshot<int> snapshot{};
    EXPECT_EQ(snapshot.Acquire(), nullptr);

    snapshot.Publish(7);
    auto first = snapshot.Acquire();
    snapshot.Publish(9);

    ASSERT_NE(first, nullptr);
    EXPECT_EQ(*first, 7);
    EXPECT_EQ(*snapshot.Acquire(), 9);

    snapshot.Cleanup();
    EXPECT_EQ(snapshot.Acquire(), nullptr);
}