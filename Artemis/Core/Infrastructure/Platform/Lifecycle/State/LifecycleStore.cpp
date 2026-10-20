module Platform.Lifecycle.State;
import :Lifecycle;

import std;

namespace
{
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    constexpr std::uint64_t k_ExpectedSkips{ 1 };

    // Longest time the game thread waits for the AI thread to read one tick. Past it the game goes on, so a stuck AI thread
    // slows the game down but never freezes it.
    constexpr milliseconds k_ReadTimeout{ 100 };
}

namespace Platform::Lifecycle::State
{
    auto LifecycleStore::IsRunning() const -> bool
    {
        return m_IsRunning.load();
    }

    auto LifecycleStore::SetRunning(bool value) -> void
    {
        m_IsRunning.store(value);

        if (!value)
        {
            this->WakeTickWaiters();
            this->WakeBlamWaiters();
            this->WakeReadWaiters();
        }
    }

    auto LifecycleStore::GetStatus() const -> Status
    {
        return m_Status.load(std::memory_order_acquire);
    }

    auto LifecycleStore::SetStatus(Status value) -> void
    {
        m_Status.store(value, std::memory_order_release);

        if (value == Status::TearingDown) this->WakeTickWaiters();
        if (value == Status::Initialized) this->WakeBlamWaiters();
    }

    auto LifecycleStore::GetShutdownMutex() const -> std::mutex&
    {
        return m_ShutdownMutex;
    }

    auto LifecycleStore::GetShutdownCV() const -> std::condition_variable&
    {
        return m_ShutdownCV;
    }

    auto LifecycleStore::SignalTick() -> bool
    {
        std::uint64_t generation{ 0 };

        {
            // The increment happens under the mutex of WaitForTick. Without it the AI thread can check the counter, miss this
            // increment and the notify before it blocks, and sleep through the tick (a lost wakeup: the read never comes, the wait
            // below times out and the tick is dropped).
            std::lock_guard<std::mutex> lock(m_TickMutex);

            // Release ensures all of Blam's logic-tick writes are visible to the
            // AI thread before it observes the incremented counter, otherwise
            // the AI thread could see the new tick but still read the previous
            // tick's data.
            generation = m_TickGeneration.fetch_add(1, std::memory_order_release) + 1;
        }

        m_TickCV.notify_one();

        if (!m_IsReaderActive.load(std::memory_order_acquire)) return true;

        std::unique_lock<std::mutex> lock(m_ReadMutex);

        return m_ReadCV.wait_for(lock, k_ReadTimeout, [&] {
            return m_ReadGeneration.load(std::memory_order_acquire) >= generation ||
                !m_IsReaderActive.load(std::memory_order_acquire) || !m_IsRunning.load();
            });
    }

    auto LifecycleStore::SetReaderActive(bool value) -> void
    {
        if (m_IsReaderActive.exchange(value, std::memory_order_acq_rel) && !value) this->WakeReadWaiters();
    }

    auto LifecycleStore::EndRead(std::uint64_t generation) -> void
    {
        {
            std::lock_guard<std::mutex> lock(m_ReadMutex);
            m_ReadGeneration.store(generation, std::memory_order_release);
        }

        m_ReadCV.notify_one();
    }

    auto LifecycleStore::WaitForTick(std::uint64_t last, std::uint64_t& dropped) -> std::uint64_t
    {
        // The use of a mutex under unique lock is necessary to make the
        // predicate evaluation and the lock of the AI thread atomic.
        std::unique_lock<std::mutex> lock(m_TickMutex);

        // Acquire ensures that all the Blam's logic-tick data is visible
        // from the AI thread when the tick generation is loaded.
        m_TickCV.wait(lock, [&] {
            return m_TickGeneration.load(std::memory_order_acquire) > last ||
                m_Status.load(std::memory_order_acquire) ==
                Status::TearingDown || !m_IsRunning.load();
            });

        std::uint64_t generation = m_TickGeneration.load(std::memory_order_acquire);

        // The expected difference between generation and last is one,
        // and that difference must not be counted as a drop.
        dropped = (generation > last) ? generation - last - k_ExpectedSkips : 0;

        return generation;
    }

    auto LifecycleStore::WakeTickWaiters() -> void
    {
        {
            std::lock_guard<std::mutex> lock(m_TickMutex);
        }

        m_TickCV.notify_all();
    }

    auto LifecycleStore::ResetTickGeneration() -> void
    {
        m_TickGeneration.store(0);
        m_ReadGeneration.store(0);
    }

    auto LifecycleStore::BeginTick() -> void
    {
        m_IsTickActive.store(true);
    }

    auto LifecycleStore::EndTick() -> void
    {
        m_IsTickActive.store(false);
    }

    auto LifecycleStore::WaitForTickEnd(milliseconds timeout) -> bool
    {
        auto deadline = steady_clock::now() + timeout;

        while (m_IsTickActive.load())
        {
            if (steady_clock::now() >= deadline) return false;
            std::this_thread::sleep_for(microseconds(100));
        }

        return true;
    }

    auto LifecycleStore::WaitForBlam() -> void
    {
        std::unique_lock<std::mutex> lock(m_BlamMutex);

        m_BlamCV.wait(lock, [&] {
            return m_Status.load(std::memory_order_acquire) ==
                Status::Initialized || !m_IsRunning.load();
            });
    }

    auto LifecycleStore::WakeBlamWaiters() -> void
    {
        {
            std::lock_guard<std::mutex> lock(m_BlamMutex);
        }

        m_BlamCV.notify_all();
    }

    auto LifecycleStore::WakeReadWaiters() -> void
    {
        {
            std::lock_guard<std::mutex> lock(m_ReadMutex);
        }

        m_ReadCV.notify_all();
    }

    auto LifecycleStore::BeginLoad() -> void
    {
        m_IsLoadActive.store(true);
    }

    auto LifecycleStore::EndLoad() -> void
    {
        m_IsLoadActive.store(false);
    }

    auto LifecycleStore::WaitForLoadEnd(milliseconds timeout) -> bool
    {
        auto deadline = steady_clock::now() + timeout;

        while (m_IsLoadActive.load())
        {
            if (steady_clock::now() >= deadline) return false;
            std::this_thread::sleep_for(microseconds(100));
        }

        return true;
    }

    auto LifecycleStore::SetRequestedTimeScale(float scale) -> void
    {
        m_RequestedTimeScale.store(scale, std::memory_order_relaxed);
    }

    auto LifecycleStore::GetRequestedTimeScale() const -> float
    {
        return m_RequestedTimeScale.load(std::memory_order_relaxed);
    }
}