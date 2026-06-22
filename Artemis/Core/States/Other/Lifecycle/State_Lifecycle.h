#pragma once

#include "Core/Types/Other/EngineStatus.h"

#include <condition_variable>
#include <cstdint>
#include <atomic>
#include <chrono>
#include <mutex>

using MilliSeconds = std::chrono::milliseconds;
using MicroSeconds = std::chrono::microseconds;
using SteadyClock = std::chrono::steady_clock;

class State_Lifecycle
{
public:
	bool IsRunning() const;
	void SetRunning(bool value);

	HMODULE GetHandleModule() const;
	void SetHandleModule(HMODULE value);

	Status GetStatus() const;
	void SetStatus(Status value);

	std::mutex& GetShutdownMutex() const;
	std::condition_variable& GetShutdownCV() const;

	/**
	* @brief Signals that Blam has just finished processing a logic-tick,
	* making the AI thread to read a stable Object Table.
	* @note Called from Blam main thread (via the SimulationTicks hook).
	* @see docs/TickSynchronization.md
	*/
	void SignalTick();

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
	uint64_t WaitForTick(uint64_t last, uint64_t& dropped);

	/**
	* @brief Returns the current tick generation that Blam 
	* has signaled so far.
	* @return The latest tick generation.
	*/
	uint64_t GetTickGeneration() const;

	/**
	* @brief Resets the tick generation to zero.
	*/
	void ResetTickGeneration();

	/**
	* @brief Marks the Artemis tick as active.
	*/
	void BeginTick();

	/**
	* @brief Marks the Artemis tick as finished.
	*/
	void EndTick();

	/**
	* @brief Waits until the AI thread finishes processing the current
	* tick, within the time window provided by `timeout`.
	* @param timeout Maximum time to wait for the tick to finish.
	* @return true if the tick was finished within the timeout.
	*/
	bool WaitForTickEnd(MilliSeconds timeout);

	/*
	* @brief Blocks the AI thread until a new Instance of Blam game
	* engine has been initialized.
	*/
	void WaitForBlam();

	/*
	* @brief Marks the resource loading as active.
	*/
	void BeginLoad();

	/*
	* @brief Marks the resource loading as finished.
	*/
	void EndLoad();

	/*
	* @brief Waits until the AI thread finishes processing the current
	* resource loading, within the time window provided by `timeout`.
	* @param timeout Maximum time to wait for the resource loading 
	* to finish.
	* @return true if the resource loading finished within the timeout.
	*/
	bool WaitForLoadEnd(MilliSeconds timeout);

private:
	std::atomic<bool> m_IsRunning{ false };
	std::atomic<HMODULE> m_HandleModule{ nullptr };
	std::atomic<Status> m_Status{ Status::Waiting };

	mutable std::condition_variable m_ShutdownCV{};
	mutable std::mutex m_ShutdownMutex{};

	static constexpr uint64_t k_ExpectedSkips = 1;

	std::atomic<bool> m_IsTickActive{ false };
	std::atomic<bool> m_IsLoadActive{ false };

	std::atomic<uint64_t> m_TickGeneration{ 0 };
	std::condition_variable m_TickCV;
	std::mutex m_TickMutex;

	std::condition_variable m_BlamCV;
	std::mutex m_BlamMutex;

	/**
	* @brief Wakes all threads blocked on `m_TickCV`,
	* forcing them to re-check their predicate.
	*/
	void WakeTickWaiters();

	/*
	* @brief Wakes all the threads blocked on `m_BlamCV`,
	* forcing them to re-check their predicate.
	*/
	void WakeBlamWaiters();
};