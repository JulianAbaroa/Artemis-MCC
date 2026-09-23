module Platform.Lifecycle.State;

namespace
{
	using MicroSeconds = std::chrono::microseconds;
	using SteadyClock = std::chrono::steady_clock;
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
		}
	}

	auto LifecycleStore::GetHandleModule() const -> void*
	{
		return m_HandleModule.load();
	}

	auto LifecycleStore::SetHandleModule(void* value) -> void
	{
		m_HandleModule.store(value);
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

	auto LifecycleStore::SignalTick() -> void
	{
		// Release ensures all of Blam's logic-tick writes are visible to the
		// AI thread before it observes the incremented counter, otherwise 
		// the AI thread could see the new tick but still read the previous
		// tick's data.
		m_TickGeneration.fetch_add(1, std::memory_order_release);
		m_TickCV.notify_one();
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
		dropped = (generation > last) ?
			generation - last - k_ExpectedSkips : 0;

		return generation;
	}

	auto LifecycleStore::WakeTickWaiters() -> void
	{
		{
			std::lock_guard<std::mutex> lock(m_TickMutex);
		}

		m_TickCV.notify_all();
	}

	auto LifecycleStore::GetTickGeneration() const -> std::uint64_t
	{
		return m_TickGeneration.load();
	}

	auto LifecycleStore::ResetTickGeneration() -> void
	{
		m_TickGeneration.store(0);
	}

	auto LifecycleStore::BeginTick() -> void
	{
		m_IsTickActive.store(true);
	}

	auto LifecycleStore::EndTick() -> void
	{
		m_IsTickActive.store(false);
	}

	auto LifecycleStore::WaitForTickEnd(MilliSeconds timeout) -> bool
	{
		auto deadline = SteadyClock::now() + timeout;

		while (m_IsTickActive.load())
		{
			if (SteadyClock::now() >= deadline) return false;
			std::this_thread::sleep_for(MicroSeconds(100));
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

	auto LifecycleStore::BeginLoad() -> void
	{
		m_IsLoadActive.store(true);
	}

	auto LifecycleStore::EndLoad() -> void
	{
		m_IsLoadActive.store(false);
	}

	auto LifecycleStore::WaitForLoadEnd(MilliSeconds timeout) -> bool
	{
		auto deadline = SteadyClock::now() + timeout;

		while (m_IsLoadActive.load())
		{
			if (SteadyClock::now() >= deadline) return false;
			std::this_thread::sleep_for(MicroSeconds(100));
		}

		return true;
	}
}