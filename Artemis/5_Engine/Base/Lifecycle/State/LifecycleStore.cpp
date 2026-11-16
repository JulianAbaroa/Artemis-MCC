module Engine.Base.Lifecycle.State;
import :Lifecycle;
import std;

namespace
{
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
}

namespace Engine::Base::Lifecycle::State
{
    auto LifecycleStore::IsRunning() const -> bool
    {
        return m_IsRunning.load();
    }

    auto LifecycleStore::SetRunning(bool value) -> void
    {
        m_IsRunning.store(value);

        if (!value) this->WakeBlamWaiters();
    }

    auto LifecycleStore::GetStatus() const -> Status
    {
        return m_Status.load(std::memory_order_acquire);
    }

    auto LifecycleStore::SetStatus(Status value) -> void
    {
        // The epoch goes up before the status, so whoever sees TearingDown or Destroyed also sees the new epoch.
        if (value == Status::TearingDown) m_TeardownEpoch.fetch_add(1, std::memory_order_acq_rel);

        m_Status.store(value, std::memory_order_release);

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

    auto LifecycleStore::BeginTick() -> void
    {
        m_IsTickActive.store(true);

        // Store then load across threads (the status the caller reads next): only a full fence orders them on x86.
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    auto LifecycleStore::EndTick() -> void
    {
        m_IsTickActive.store(false);
    }

    auto LifecycleStore::WaitForTickEnd(milliseconds timeout) -> bool
    {
        auto deadline = steady_clock::now() + timeout;

        // The status the caller stored before (TearingDown) is visible to a tick that starts after this load, see BeginTick.
        std::atomic_thread_fence(std::memory_order_seq_cst);

        while (m_IsTickActive.load())
        {
            if (steady_clock::now() >= deadline) return false;
            std::this_thread::sleep_for(microseconds(100));
        }

        return true;
    }

    auto LifecycleStore::CanPublish() const -> bool
    {
        const Status status{ m_Status.load(std::memory_order_acquire) };

        return status != Status::TearingDown && status != Status::Destroyed;
    }

    auto LifecycleStore::GetTeardownEpoch() const -> std::uint64_t
    {
        return m_TeardownEpoch.load(std::memory_order_acquire);
    }

    auto LifecycleStore::MarkLoaded(std::uint64_t epoch) -> void
    {
        m_LoadedEpoch.store(epoch, std::memory_order_release);
    }

    auto LifecycleStore::GetLoadedEpoch() const -> std::uint64_t
    {
        return m_LoadedEpoch.load(std::memory_order_acquire);
    }

    auto LifecycleStore::AddTeardownTimeout() -> void
    {
        m_TeardownTimeouts.fetch_add(1, std::memory_order_relaxed);
    }

    auto LifecycleStore::GetTeardownTimeouts() const -> std::uint64_t
    {
        return m_TeardownTimeouts.load(std::memory_order_relaxed);
    }

    auto LifecycleStore::AddDiscardedTick() -> void
    {
        m_DiscardedTicks.fetch_add(1, std::memory_order_relaxed);
    }

    auto LifecycleStore::GetDiscardedTicks() const -> std::uint64_t
    {
        return m_DiscardedTicks.load(std::memory_order_relaxed);
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

    auto LifecycleStore::SetPauseRequested(bool value) -> void
    {
        m_IsPauseRequested.store(value, std::memory_order_relaxed);
    }

    auto LifecycleStore::IsPauseRequested() const -> bool
    {
        return m_IsPauseRequested.load(std::memory_order_relaxed);
    }

    auto LifecycleStore::SetEndRoundRequested(bool value) -> void
    {
        m_IsEndRoundRequested.store(value, std::memory_order_relaxed);
    }

    auto LifecycleStore::TakeEndRound() -> bool
    {
        return m_IsEndRoundRequested.exchange(false, std::memory_order_relaxed);
    }

    auto LifecycleStore::AddPausedFrame() -> void
    {
        m_PausedFrames.fetch_add(1, std::memory_order_relaxed);
    }

    auto LifecycleStore::GetPausedFrames() const -> std::uint64_t
    {
        return m_PausedFrames.load(std::memory_order_relaxed);
    }
}