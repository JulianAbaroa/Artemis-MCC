#include <gtest/gtest.h>

import Engine.Capture.Oracle.Type;
import Engine.Capture.Oracle.State;
import std;

// OracleStore (R3, D272): the hand-over of rays and answers between the AI thread and the game thread. The game thread never blocks on it.

namespace
{
    using Engine::Capture::Oracle::State::OracleStore;
    using Engine::Capture::Oracle::Type::OracleBatch;
    using Engine::Capture::Oracle::Type::OracleOutcome;
    using Engine::Capture::Oracle::Type::OracleRay;
    using Engine::Capture::Oracle::Type::OracleStatus;
    using Engine::Capture::Oracle::Type::k_MaxPendingOutcomes;

    auto MakeBatch(std::uint64_t id, std::size_t rays) -> OracleBatch
    {
        OracleBatch batch{ .Id = id };
        batch.Rays.resize(rays);
        return batch;
    }

    auto MakeOutcome(std::uint64_t id) -> OracleOutcome
    {
        return OracleOutcome{ .BatchId = id };
    }
}

TEST(OracleStore, StartsOffAndUnused)
{
    OracleStore store{};

    EXPECT_FALSE(store.IsEnabled());
    EXPECT_EQ(store.GetStatus(), OracleStatus::Off);
    EXPECT_EQ(store.GetSequence(), 0u);
    EXPECT_FALSE(store.TakeBatch().has_value());
    EXPECT_TRUE(store.TakeOutcomes().empty());
}

TEST(OracleStore, EnabledAndStatusAreSetAndRead)
{
    OracleStore store{};
    store.SetEnabled(true);
    store.SetStatus(OracleStatus::Running);

    EXPECT_TRUE(store.IsEnabled());
    EXPECT_EQ(store.GetStatus(), OracleStatus::Running);
}

TEST(OracleStore, StatusTransitionOnlyFromTheExpectedValue)
{
    OracleStore store{};
    store.SetStatus(OracleStatus::Unavailable);

    EXPECT_FALSE(store.TryTransition(OracleStatus::Off, OracleStatus::Waiting));
    EXPECT_EQ(store.GetStatus(), OracleStatus::Unavailable);

    store.SetStatus(OracleStatus::Waiting);
    EXPECT_TRUE(store.TryTransition(OracleStatus::Waiting, OracleStatus::Running));
    EXPECT_FALSE(store.TryTransition(OracleStatus::Waiting, OracleStatus::Off));
    EXPECT_EQ(store.GetStatus(), OracleStatus::Running);
}

TEST(OracleStore, SequenceAndBatchIdsOnlyGoUp)
{
    OracleStore store{};

    EXPECT_EQ(store.AdvanceSequence(), 1u);
    EXPECT_EQ(store.AdvanceSequence(), 2u);
    EXPECT_EQ(store.GetSequence(), 2u);
    EXPECT_NE(store.AllocateBatchId(), store.AllocateBatchId());
}

TEST(OracleStore, ABatchIsTakenOnce)
{
    OracleStore store{};
    store.Submit(MakeBatch(5, 3));

    const std::optional<OracleBatch> batch{ store.TakeBatch() };

    ASSERT_TRUE(batch.has_value());
    EXPECT_EQ(batch->Id, 5u);
    EXPECT_EQ(batch->Rays.size(), 3u);
    EXPECT_FALSE(store.TakeBatch().has_value());
}

TEST(OracleStore, ANewBatchReplacesTheOneNotTakenAndCountsTheDrop)
{
    OracleStore store{};
    store.Submit(MakeBatch(1, 1));
    store.Submit(MakeBatch(2, 1));

    const std::optional<OracleBatch> batch{ store.TakeBatch() };

    ASSERT_TRUE(batch.has_value());
    EXPECT_EQ(batch->Id, 2u);
    EXPECT_EQ(store.GetDroppedBatches(), 1u);
}

TEST(OracleStore, OutcomesComeOutOldestFirstAndAreTakenOnce)
{
    OracleStore store{};
    store.Publish(MakeOutcome(1));
    store.Publish(MakeOutcome(2));

    const std::vector<OracleOutcome> outcomes{ store.TakeOutcomes() };

    ASSERT_EQ(outcomes.size(), 2u);
    EXPECT_EQ(outcomes[0].BatchId, 1u);
    EXPECT_EQ(outcomes[1].BatchId, 2u);
    EXPECT_TRUE(store.TakeOutcomes().empty());
}

TEST(OracleStore, TooManyOutcomesDropTheOldest)
{
    OracleStore store{};

    for (std::uint64_t id{ 1 }; id <= k_MaxPendingOutcomes + 2; ++id) store.Publish(MakeOutcome(id));

    const std::vector<OracleOutcome> outcomes{ store.TakeOutcomes() };

    ASSERT_EQ(outcomes.size(), k_MaxPendingOutcomes);
    EXPECT_EQ(outcomes.front().BatchId, 3u);
    EXPECT_EQ(outcomes.back().BatchId, k_MaxPendingOutcomes + 2);
    EXPECT_EQ(store.GetDroppedOutcomes(), 2u);
}

TEST(OracleStore, CleanupForgetsBatchAndOutcomes)
{
    OracleStore store{};
    store.Submit(MakeBatch(1, 1));
    store.Publish(MakeOutcome(1));

    store.Cleanup();

    EXPECT_FALSE(store.TakeBatch().has_value());
    EXPECT_TRUE(store.TakeOutcomes().empty());
}

TEST(OracleStore, TheGameThreadNeverBlocksOnABusyLock)
{
    OracleStore store{};
    store.Submit(MakeBatch(1, 1));

    // The AI thread holds the lock inside TakeOutcomes only for a moment: the game thread calls must return without waiting in any case.
    std::atomic<bool> isDone{ false };
    std::jthread aiThread{ [&] {
        while (!isDone.load()) store.TakeOutcomes();
    } };

    for (int index{ 0 }; index < 2000; ++index)
    {
        store.TakeBatch();
        store.Publish(MakeOutcome(static_cast<std::uint64_t>(index)));
    }

    isDone.store(true);
    SUCCEED();
}