#include <gtest/gtest.h>

import Engine.Base.Lifecycle.State;
import Engine.Base.Lifecycle.Type;
import std;

// LifecycleStore (M6c.2, D221): the handshake between the game thread and the AI thread, and the flags the bridge and the UI write.
// The tests fix what the code does today. Times use steady_clock with wide margins, and threads are std::jthread.

namespace
{
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

TEST_F(LifecycleStoreTests, SignalTickWithoutReaderReturnsAtOnce)
{
    bool signaled{ false };
    const double elapsed{ TimeOf([&] { signaled = m_Store.SignalTick(); }) };

    EXPECT_TRUE(signaled);
    EXPECT_LT(elapsed, 20.0);

    std::uint64_t dropped{ 99 };
    EXPECT_EQ(m_Store.WaitForTick(0, dropped), 1u);
    EXPECT_EQ(dropped, 0u);
}

TEST_F(LifecycleStoreTests, ReaderEndsTheWindow)
{
    m_Store.SetReaderActive(true);

    std::jthread reader{ [this] {
        std::uint64_t dropped{ 0 };
        const std::uint64_t generation{ m_Store.WaitForTick(0, dropped) };
        std::this_thread::sleep_for(milliseconds{ 20 });
        m_Store.EndRead(generation);
    } };

    bool signaled{ false };
    const double elapsed{ TimeOf([&] { signaled = m_Store.SignalTick(); }) };

    EXPECT_TRUE(signaled);
    EXPECT_GE(elapsed, 15.0);
    EXPECT_LT(elapsed, 90.0);
}

TEST_F(LifecycleStoreTests, ReadTimeoutDropsTheTick)
{
    m_Store.SetReaderActive(true);

    bool signaled{ true };
    const double elapsed{ TimeOf([&] { signaled = m_Store.SignalTick(); }) };

    EXPECT_FALSE(signaled);
    EXPECT_GE(elapsed, 90.0);
    EXPECT_LT(elapsed, 300.0);
}

TEST_F(LifecycleStoreTests, WaitForTickCountsDrops)
{
    m_Store.SignalTick();
    m_Store.SignalTick();
    m_Store.SignalTick();

    std::uint64_t dropped{ 0 };
    EXPECT_EQ(m_Store.WaitForTick(0, dropped), 3u);
    EXPECT_EQ(dropped, 2u);

    std::uint64_t waiterDropped{ 99 };
    std::uint64_t waiterGeneration{ 0 };
    std::jthread waiter{ [&] { waiterGeneration = m_Store.WaitForTick(3, waiterDropped); } };

    std::this_thread::sleep_for(milliseconds{ 20 });
    m_Store.SignalTick();
    waiter.join();

    EXPECT_EQ(waiterGeneration, 4u);
    EXPECT_EQ(waiterDropped, 0u);
}

TEST_F(LifecycleStoreTests, StopWakesTheWaiters)
{
    m_Store.SignalTick();

    // A waiter on a tick that never comes ends when the store stops.
    {
        std::uint64_t dropped{ 99 };
        std::jthread waiter{ [&] { m_Store.WaitForTick(1, dropped); } };

        std::this_thread::sleep_for(milliseconds{ 30 });
        const double elapsed{ TimeOf([&] { m_Store.SetRunning(false); waiter.join(); }) };

        EXPECT_LT(elapsed, 200.0);
        EXPECT_EQ(dropped, 0u);
    }

    // Same when the engine starts tearing down.
    m_Store.SetRunning(true);
    {
        std::uint64_t dropped{ 99 };
        std::jthread waiter{ [&] { m_Store.WaitForTick(1, dropped); } };

        std::this_thread::sleep_for(milliseconds{ 30 });
        const double elapsed{ TimeOf([&] { m_Store.SetStatus(Status::TearingDown); waiter.join(); }) };

        EXPECT_LT(elapsed, 200.0);
        EXPECT_EQ(dropped, 0u);
    }

    // WaitForBlam ends when the engine is initialized.
    m_Store.SetStatus(Status::Waiting);
    {
        std::jthread waiter{ [&] { m_Store.WaitForBlam(); } };

        std::this_thread::sleep_for(milliseconds{ 30 });
        const double elapsed{ TimeOf([&] { m_Store.SetStatus(Status::Initialized); waiter.join(); }) };

        EXPECT_LT(elapsed, 200.0);
    }

    // The game thread leaves SignalTick when the reader goes away.
    m_Store.SetReaderActive(true);
    {
        std::jthread stopper{ [&] {
            std::this_thread::sleep_for(milliseconds{ 10 });
            m_Store.SetReaderActive(false);
        } };

        bool signaled{ false };
        const double elapsed{ TimeOf([&] { signaled = m_Store.SignalTick(); }) };

        EXPECT_TRUE(signaled);
        EXPECT_LT(elapsed, 50.0);
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

TEST_F(LifecycleStoreTests, ResetTickGeneration)
{
    m_Store.SignalTick();
    m_Store.SignalTick();
    m_Store.ResetTickGeneration();
    m_Store.SignalTick();

    std::uint64_t dropped{ 99 };
    EXPECT_EQ(m_Store.WaitForTick(0, dropped), 1u);
    EXPECT_EQ(dropped, 0u);
}