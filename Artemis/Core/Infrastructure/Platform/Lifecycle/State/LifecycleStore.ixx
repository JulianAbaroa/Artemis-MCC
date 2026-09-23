export module Platform.Lifecycle.State;

import Platform.Lifecycle.Type;
import std;

export namespace Platform::Lifecycle::State
{
	class LifecycleStore
	{
	private:
		using Status = Platform::Lifecycle::Type::Status;
		using MilliSeconds = std::chrono::milliseconds;

		static constexpr std::uint64_t k_ExpectedSkips = 1;

	public:
		LifecycleStore() = default;
		~LifecycleStore() = default;

		auto IsRunning() const -> bool;
		auto SetRunning(bool value) -> void;

		auto GetHandleModule() const -> void*;
		auto SetHandleModule(void* value) -> void;

		auto GetStatus() const -> Status;
		auto SetStatus(Status value) -> void;

		auto GetShutdownMutex() const -> std::mutex&;
		auto GetShutdownCV() const -> std::condition_variable&;

		/**
		* @brief Signals that Blam has just finished processing a logic-tick,
		* making the AI thread to read a stable Object Table.
		* @note Called from Blam main thread (via the SimulationTicks hook).
		* @see docs/TickSynchronization.md
		*/
		auto SignalTick() -> void;

		/**
		* @brief Blocks the AI thread until a new logic-tick is signaled,
		* the mod stops running, or Blam game engine begins tearing down.
		* @param last The last tick generation the AI thread already processed.
		* @param dropped [out] Number of Blam logic-ticks missed since `last`
		* (i.e. ticks that elapsed while the AI thread was busy).
		* @return The current tick generation the thread woke up on.
		* @note Called from AI thread (via the AI_Thread::Run function).
		* @see docs/TickSynchronization.md
		*/
		auto WaitForTick(std::uint64_t last, std::uint64_t& dropped) -> std::uint64_t;

		/**
		* @brief Returns the current tick generation that Blam
		* has signaled so far.
		* @return The latest tick generation.
		*/
		auto GetTickGeneration() const -> std::uint64_t;

		/**
		* @brief Resets the tick generation to zero.
		*/
		auto ResetTickGeneration() -> void;

		/**
		* @brief Marks the Artemis tick as active.
		*/
		auto BeginTick() -> void;

		/**
		* @brief Marks the Artemis tick as finished.
		*/
		auto EndTick() -> void;

		/**
		* @brief Waits until the AI thread finishes processing the current
		* tick, within the time window provided by `timeout`.
		* @param timeout Maximum time to wait for the tick to finish.
		* @return true if the tick was finished within the timeout.
		*/
		auto WaitForTickEnd(MilliSeconds timeout) -> bool;

		/*
		* @brief Blocks the AI thread until a new Instance of Blam game
		* engine has been initialized.
		*/
		auto WaitForBlam() -> void;

		/*
		* @brief Marks the resource loading as active.
		*/
		auto BeginLoad() -> void;

		/*
		* @brief Marks the resource loading as finished.
		*/
		auto EndLoad() -> void;

		/*
		* @brief Waits until the AI thread finishes processing the current
		* resource loading, within the time window provided by `timeout`.
		* @param timeout Maximum time to wait for the resource loading
		* to finish.
		* @return true if the resource loading finished within the timeout.
		*/
		auto WaitForLoadEnd(MilliSeconds timeout) -> bool;

	private:
		std::atomic<bool> m_IsRunning{ false };
		std::atomic<void*> m_HandleModule{ nullptr };
		std::atomic<Status> m_Status{ Status::Waiting };

		mutable std::condition_variable m_ShutdownCV{};
		mutable std::mutex m_ShutdownMutex{};

		std::atomic<bool> m_IsTickActive{ false };
		std::atomic<bool> m_IsLoadActive{ false };

		std::atomic<std::uint64_t> m_TickGeneration{ 0 };
		std::condition_variable m_TickCV{};
		std::mutex m_TickMutex{};

		std::condition_variable m_BlamCV{};
		std::mutex m_BlamMutex{};

		/**
		* @brief Wakes all threads blocked on `m_TickCV`,
		* forcing them to re-check their predicate.
		*/
		auto WakeTickWaiters() -> void;

		/*
		* @brief Wakes all the threads blocked on `m_BlamCV`,
		* forcing them to re-check their predicate.
		*/
		auto WakeBlamWaiters() -> void;
	};
}