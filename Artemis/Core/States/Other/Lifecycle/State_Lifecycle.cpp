#include "pch.h"

#include "State_Lifecycle.h"

bool State_Lifecycle::IsRunning() const 
{ 
	return m_IsRunning.load(); 
}

void State_Lifecycle::SetRunning(bool value) 
{ 
	m_IsRunning.store(value);

	if (!value) this->WakeTickWaiters();
}

HMODULE State_Lifecycle::GetHandleModule() const 
{ 
	return m_HandleModule.load(); 
}

void State_Lifecycle::SetHandleModule(HMODULE value) 
{ 
	m_HandleModule.store(value); 
}

Status State_Lifecycle::GetStatus() const 
{ 
	return m_Status.load(std::memory_order_acquire); 
}

void State_Lifecycle::SetStatus(Status value) 
{ 
	m_Status.store(value, std::memory_order_release); 

	if (value == Status::TearingDown) this->WakeTickWaiters();
	if (value == Status::Initialized) this->WakeBlamWaiters();
}

std::mutex& State_Lifecycle::GetShutdownMutex() const
{
	return m_ShutdownMutex;
}

std::condition_variable& State_Lifecycle::GetShutdownCV() const
{
	return m_ShutdownCV;
}

void State_Lifecycle::SignalTick()
{
	// Release ensures all of Blam's logic-tick writes are visible to the
	// AI thread before it observes the incremented counter, otherwise 
	// the AI thread could see the new tick but still read the previous
	// tick's data.
	m_TickGeneration.fetch_add(1, std::memory_order_release);
	m_TickCV.notify_one();
}

uint64_t State_Lifecycle::WaitForTick(uint64_t last, uint64_t& dropped)
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

	uint64_t generation = m_TickGeneration.load(std::memory_order_acquire);

	// The expected difference between generation and last is one,
	// and that difference must not be counted as a drop.
	dropped = (generation > last) ? 
		generation - last - k_ExpectedSkips : 0;

	return generation;
}

void State_Lifecycle::WakeTickWaiters()
{
	m_TickCV.notify_all();
}

uint64_t State_Lifecycle::GetTickGeneration() const
{
	return m_TickGeneration.load();
}

void State_Lifecycle::ResetTickGeneration()
{
	m_TickGeneration.store(0);
}

void State_Lifecycle::BeginTick() 
{ 
	m_IsTickActive.store(true); 
}

void State_Lifecycle::EndTick() 
{ 
	m_IsTickActive.store(false); 
}

bool State_Lifecycle::WaitForTickEnd(MilliSeconds timeout)
{
	auto deadline = SteadyClock::now() + timeout;

	while (m_IsTickActive.load())
	{
		if (SteadyClock::now() >= deadline) return false;
		std::this_thread::sleep_for(MicroSeconds(100));
	}

	return true;
}

void State_Lifecycle::WaitForBlam()
{
	std::unique_lock<std::mutex> lock(m_BlamMutex);

	m_BlamCV.wait(lock, [&] {
		return m_Status.load(std::memory_order_acquire) == 
			Status::Initialized || !m_IsRunning.load();
	});
}

void State_Lifecycle::WakeBlamWaiters()
{
	m_BlamCV.notify_all();
}

void State_Lifecycle::BeginLoad()
{
	m_IsLoadActive.store(true);
}

void State_Lifecycle::EndLoad()
{
	m_IsLoadActive.store(false);
}

bool State_Lifecycle::WaitForLoadEnd(MilliSeconds timeout)
{
	auto deadline = SteadyClock::now() + timeout;

	while (m_IsLoadActive.load())
	{
		if (SteadyClock::now() >= deadline) return false;
		std::this_thread::sleep_for(MicroSeconds(100));
	}

	return true;
}