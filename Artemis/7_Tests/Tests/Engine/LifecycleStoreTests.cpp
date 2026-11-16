#include <gtest/gtest.h>

import Common.State.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.Type;
import std;

// LifecycleStore (M6c.2, D221): the lifecycle, the tick and load windows of the teardown and the flags the bridge and the UI write. The
// tick handshake is the queue of D305 (BoundedQueue), tested here only as far as the teardown depends on it.
// The tests fix what the code does today. Times use steady_clock with wide margins, and threads are std::jthread.

namespace
{
    using Common::State::Type::BoundedQueue;
    using Engine::Base::Lifecycle::State::LifecycleStore;
    using Engine::Base::Lifecycle::Type::Status;

    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    // The store starts not running. The fixture runs it, and stops it at the end so no waiter outlives the test.
    class LifecycleStoreTests : public testing::Test
    {
    protected:
        LifecycleStore m_Store{};

        auto SetUp() -> void override
        {
            m_Store.SetRunning(true);
        }

        auto TearDown() -> void override
        {
            m_Store.SetRunning(false);
        }

        // return: Milliseconds the call takes.
        template <typename Call>
        static auto TimeOf(Call&& call) -> double
        {
            const auto start{ steady_clock::now() };
            call();
            return std::chrono::duration<double, std::milli>(steady_clock::now() - start).count();
        }
    };
}

TEST_F(LifecycleStoreTests, WaitForBlamEndsWhenTheEngineIsInitializedOrArtemisStops)
{
    m_Store.SetStatus(Status::Waiting);
    {
        std::jthread waiter{ [&] { m_Store.WaitForBlam(); } };

        std::this_thread::sleep_for(milliseconds{ 30 });
        const double elapsed{ TimeOf([&] { m_Store.SetStatus(Status::Initialized); waiter.join(); }) };

        EXPECT_LT(elapsed, 200.0);
    }

    m_Store.SetStatus(Status::Waiting);
    {
        std::jthread waiter{ [&] { m_Store.WaitForBlam(); } };

        std::this_thread::sleep_for(milliseconds{ 30 });
        const double elapsed{ TimeOf([&] { m_Store.SetRunning(false); waiter.join(); }) };

        EXPECT_LT(elapsed, 200.0);
    }
}

TEST_F(LifecycleStoreTests, EndRoundIsTakenOnce)
{
    m_Store.SetEndRoundRequested(true);
    EXPECT_TRUE(m_Store.TakeEndRound());
    EXPECT_FALSE(m_Store.TakeEndRound());

    m_Store.AddPausedFrame();
    m_Store.AddPausedFrame();
    EXPECT_EQ(m_Store.GetPausedFrames(), 2u);

    m_Store.SetRequestedTimeScale(2.5f);
    EXPECT_EQ(m_Store.GetRequestedTimeScale(), 2.5f);
    m_Store.SetPauseRequested(true);
    EXPECT_TRUE(m_Store.IsPauseRequested());
    m_Store.SetPauseRequested(false);
    EXPECT_FALSE(m_Store.IsPauseRequested());
}

TEST_F(LifecycleStoreTests, TickAndLoadWindows)
{
    m_Store.BeginTick();
    EXPECT_FALSE(m_Store.WaitForTickEnd(milliseconds{ 20 }));
    m_Store.EndTick();
    EXPECT_TRUE(m_Store.WaitForTickEnd(milliseconds{ 20 }));

    m_Store.BeginLoad();
    EXPECT_FALSE(m_Store.WaitForLoadEnd(milliseconds{ 20 }));
    m_Store.EndLoad();
    EXPECT_TRUE(m_Store.WaitForLoadEnd(milliseconds{ 20 }));
}

TEST_F(LifecycleStoreTests, CanPublishOnlyBeforeTheTeardown)
{
    for (const Status status : { Status::Waiting, Status::Initialized, Status::Running })
    {
        m_Store.SetStatus(status);
        EXPECT_TRUE(m_Store.CanPublish());
    }

    for (const Status status : { Status::TearingDown, Status::Destroyed })
    {
        m_Store.SetStatus(status);
        EXPECT_FALSE(m_Store.CanPublish());
    }
}

TEST_F(LifecycleStoreTests, TeardownCountersStartAtZeroAndCount)
{
    EXPECT_EQ(m_Store.GetTeardownTimeouts(), 0u);
    EXPECT_EQ(m_Store.GetDiscardedTicks(), 0u);

    m_Store.AddTeardownTimeout();
    m_Store.AddDiscardedTick();
    m_Store.AddDiscardedTick();

    EXPECT_EQ(m_Store.GetTeardownTimeouts(), 1u);
    EXPECT_EQ(m_Store.GetDiscardedTicks(), 2u);
}

// F1 C.7: the game thread does not wait for a long tick past its timeout, and the tick finds the state changed when it ends, so it
// publishes nothing and is counted. This is the order AIThread::Run follows (BeginTick, work, CanPublish, publish or discard, EndTick).
TEST_F(LifecycleStoreTests, TickRunningAtTeardownDoesNotPublish)
{
    m_Store.SetStatus(Status::Running);
    m_Store.BeginTick();

    std::atomic<bool> release{ false };
    std::atomic<bool> published{ false };
    std::jthread tick{ [&] {
        while (!release.load()) std::this_thread::sleep_for(milliseconds{ 1 });

        if (m_Store.CanPublish()) published.store(true);
        else m_Store.AddDiscardedTick();
        m_Store.EndTick();
    } };

    // The detour: TearingDown, a wait that expires, the Cleanup goes on. The detour waits 1 s (k_TickEndTimeout); 10 ms is a short test
    // value, the tick is held on purpose so the wait expires.
    m_Store.SetStatus(Status::TearingDown);
    if (!m_Store.WaitForTickEnd(milliseconds{ 10 })) m_Store.AddTeardownTimeout();

    release.store(true);
    tick.join();

    EXPECT_FALSE(published.load());
    EXPECT_EQ(m_Store.GetTeardownTimeouts(), 1u);
    EXPECT_EQ(m_Store.GetDiscardedTicks(), 1u);
    EXPECT_TRUE(m_Store.WaitForTickEnd(milliseconds{ 20 }));
}

TEST_F(LifecycleStoreTests, TickThatEndsBeforeTheTeardownPublishes)
{
    m_Store.SetStatus(Status::Running);
    m_Store.BeginTick();

    const bool canPublish{ m_Store.CanPublish() };
    m_Store.EndTick();
    m_Store.SetStatus(Status::TearingDown);

    EXPECT_TRUE(canPublish);
    EXPECT_TRUE(m_Store.WaitForTickEnd(milliseconds{ 10 }));
    EXPECT_EQ(m_Store.GetTeardownTimeouts(), 0u);
}

TEST_F(LifecycleStoreTests, TeardownEpochGoesUpOnlyWithTearingDown)
{
    EXPECT_EQ(m_Store.GetTeardownEpoch(), 0u);

    for (const Status status : { Status::Initialized, Status::Running, Status::Destroyed, Status::Waiting })
    {
        m_Store.SetStatus(status);
        EXPECT_EQ(m_Store.GetTeardownEpoch(), 0u);
    }

    m_Store.SetStatus(Status::TearingDown);
    EXPECT_EQ(m_Store.GetTeardownEpoch(), 1u);

    // A thread that only saw Destroyed (it outlasted the teardown) still finds the epoch changed, and so does one that slept until
    // the next Initialized.
    m_Store.SetStatus(Status::Destroyed);
    m_Store.SetStatus(Status::Initialized);
    EXPECT_EQ(m_Store.GetTeardownEpoch(), 1u);

    m_Store.SetStatus(Status::TearingDown);
    EXPECT_EQ(m_Store.GetTeardownEpoch(), 2u);
}

TEST_F(LifecycleStoreTests, NothingIsLoadedUntilTheAIThreadSaysSoAndAnEpochOnlyMatchesItsOwnLoad)
{
    // Before any load no epoch matches, not even the first one.
    EXPECT_NE(m_Store.GetLoadedEpoch(), m_Store.GetTeardownEpoch());

    m_Store.MarkLoaded(m_Store.GetTeardownEpoch());
    EXPECT_EQ(m_Store.GetLoadedEpoch(), m_Store.GetTeardownEpoch());

    // A teardown raises the epoch: the resources of the old map never match the next one, so the game thread queues nothing for it.
    m_Store.SetStatus(Status::TearingDown);
    EXPECT_NE(m_Store.GetLoadedEpoch(), m_Store.GetTeardownEpoch());

    m_Store.MarkLoaded(m_Store.GetTeardownEpoch());
    EXPECT_EQ(m_Store.GetLoadedEpoch(), m_Store.GetTeardownEpoch());
}

// D305: the game thread produces, the AI thread consumes, and the teardown runs on the game thread while ticks are still queued. The
// consumer follows the order of AIThread::ProcessTick: pop, BeginTick, check the epoch, work or discard, EndTick.
TEST_F(LifecycleStoreTests, TeardownWithQueuedTicksDoesNotWaitForThemAndWaitsForTheOneInProgress)
{
    m_Store.SetStatus(Status::Running);
    const std::uint64_t epoch{ m_Store.GetTeardownEpoch() };
    m_Store.MarkLoaded(epoch);

    BoundedQueue<std::uint64_t> queue{ 4 };
    queue.SetOpen(true);
    for (std::uint64_t generation{ 1 }; generation <= 4; ++generation) ASSERT_TRUE(queue.Push(generation, milliseconds{ 10 }).IsQueued);

    // The AI thread takes the first tick and is still working on it when the teardown starts.
    const std::optional<std::uint64_t> inProgress{ queue.WaitPop(milliseconds{ 10 }) };
    ASSERT_TRUE(inProgress.has_value());
    m_Store.BeginTick();

    std::atomic<bool> release{ false };
    std::jthread consumer{ [&] {
        while (!release.load()) std::this_thread::sleep_for(milliseconds{ 1 });
        m_Store.EndTick();
    } };

    // The game thread: TearingDown, then the wait of the detour. The three ticks still queued are not waited for, the one in progress is.
    m_Store.SetStatus(Status::TearingDown);
    EXPECT_FALSE(m_Store.WaitForTickEnd(milliseconds{ 10 }));
    EXPECT_NE(m_Store.GetLoadedEpoch(), m_Store.GetTeardownEpoch());

    release.store(true);
    consumer.join();
    EXPECT_TRUE(m_Store.WaitForTickEnd(milliseconds{ 20 }));
    EXPECT_EQ(queue.Size(), 3u);

    // The AI thread resets: what is pending counts as discarded and no tick of the old epoch is processed.
    const std::size_t pending{ queue.Clear() };
    for (std::size_t i{ 0 }; i < pending; ++i) m_Store.AddDiscardedTick();
    EXPECT_EQ(m_Store.GetDiscardedTicks(), 3u);
    EXPECT_EQ(queue.Size(), 0u);
}

TEST_F(LifecycleStoreTests, TickTakenFromTheQueueAfterTheTeardownBeganSeesItAndIsDiscarded)
{
    m_Store.SetStatus(Status::Running);
    const std::uint64_t tickEpoch{ m_Store.GetTeardownEpoch() };

    // The consumer popped the tick, then the game thread set TearingDown and found no tick in progress: the Cleanup goes on.
    m_Store.SetStatus(Status::TearingDown);
    EXPECT_TRUE(m_Store.WaitForTickEnd(milliseconds{ 10 }));

    // The consumer starts the tick now: the epoch or CanPublish tell it, so it discards instead of reading the stores the Cleanup empties.
    m_Store.BeginTick();
    const bool isStale{ tickEpoch != m_Store.GetTeardownEpoch() || !m_Store.CanPublish() };
    EXPECT_TRUE(isStale);
    m_Store.AddDiscardedTick();
    m_Store.EndTick();

    EXPECT_EQ(m_Store.GetDiscardedTicks(), 1u);
}

TEST_F(LifecycleStoreTests, ProducerAndConsumerKeepTheOrderAcrossAThreadPair)
{
    m_Store.SetStatus(Status::Running);
    m_Store.MarkLoaded(m_Store.GetTeardownEpoch());

    BoundedQueue<std::uint64_t> queue{ 4 };
    queue.SetOpen(true);

    constexpr std::uint64_t k_Ticks{ 500 };
    std::vector<std::uint64_t> seen{};
    std::jthread consumer{ [&] {
        while (seen.size() < k_Ticks)
        {
            const std::optional<std::uint64_t> tick{ queue.WaitPop(milliseconds{ 50 }) };
            if (!tick.has_value()) continue;

            m_Store.BeginTick();
            seen.push_back(*tick);
            if (*tick % 50 == 0) std::this_thread::sleep_for(milliseconds{ 1 });
            m_Store.EndTick();
        }
    } };

    std::uint64_t lost{ 0 };
    for (std::uint64_t generation{ 1 }; generation <= k_Ticks; ++generation)
    {
        if (!queue.Push(generation, milliseconds{ 2000 }).IsQueued) ++lost;
    }
    consumer.join();

    EXPECT_EQ(lost, 0u);
    ASSERT_EQ(seen.size(), k_Ticks);
    EXPECT_TRUE(std::ranges::is_sorted(seen));
    EXPECT_TRUE(m_Store.WaitForTickEnd(milliseconds{ 20 }));
}
