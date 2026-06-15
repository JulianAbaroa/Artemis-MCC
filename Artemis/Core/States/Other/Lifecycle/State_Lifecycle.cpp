#include "pch.h"

#include "State_Lifecycle.h"

bool State_Lifecycle::IsRunning() const 
{ 
	return m_IsRunning.load(); 
}

void State_Lifecycle::SetRunning(bool value) 
{ 
	m_IsRunning.store(value); 
}

HMODULE State_Lifecycle::GetHandleModule() const 
{ 
	return m_HandleModule.load(); 
}

void State_Lifecycle::SetHandleModule(HMODULE value) 
{ 
	m_HandleModule.store(value); 
}

EngineStatus State_Lifecycle::GetEngineStatus() const 
{ 
	return m_EngineStatus.load(); 
}

void State_Lifecycle::SetEngineStatus(EngineStatus value) 
{ 
	m_EngineStatus.store(value); 
}

void State_Lifecycle::SignalTick()
{
	m_TickGen.fetch_add(1, std::memory_order_release);
	m_TickCv.notify_one();
}

uint64_t State_Lifecycle::WaitForTick(
	uint64_t lastSeen, uint64_t& droppedOut)
{
	std::unique_lock<std::mutex> lock(m_TickMtx);

	m_TickCv.wait(lock, [&] {
		return m_TickGen.load(std::memory_order_acquire) != 
			lastSeen || !this->IsRunning() || this->IsTearingDown();
		});

	uint64_t gen = m_TickGen.load(std::memory_order_acquire);
	droppedOut = (gen > lastSeen) ? (gen - lastSeen - 1) : 0;
	return gen;
}

void State_Lifecycle::WakeTickWaiters()
{
	m_TickCv.notify_all();
}

uint64_t State_Lifecycle::GetTickGen() const
{
	return m_TickGen.load(std::memory_order_acquire);
}

void State_Lifecycle::BeginAISweep() 
{ 
	m_AISweepActive.store(true, std::memory_order_release); 
}

void State_Lifecycle::EndAISweep() 
{ 
	m_AISweepActive.store(false, std::memory_order_release); 
}

bool State_Lifecycle::WaitForAIIdle(std::chrono::milliseconds timeout)
{
	auto deadline = std::chrono::steady_clock::now() + timeout;
	while (m_AISweepActive.load(std::memory_order_acquire))
	{
		if (std::chrono::steady_clock::now() >= deadline)
			return false;
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
	return true;
}

void State_Lifecycle::SetTearingDown(bool v)
{ 
	m_EngineTearingDown.store(v, std::memory_order_release); 
}

bool State_Lifecycle::IsTearingDown() const 
{ 
	return m_EngineTearingDown.load(std::memory_order_acquire); 
}

std::mutex& State_Lifecycle::GetMutex() const 
{ 
	return m_Mutex; 
}

std::condition_variable& State_Lifecycle::GetCV() const 
{ 
	return m_ShutdownCV; 
}