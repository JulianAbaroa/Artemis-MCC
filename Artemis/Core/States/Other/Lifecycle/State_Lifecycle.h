#pragma once

#include "Core/Types/Other/EngineStatus.h"

#include <condition_variable>
#include <cstdint>
#include <atomic>
#include <mutex>

class State_Lifecycle
{
public:
	bool IsRunning() const;
	void SetRunning(bool value);

	HMODULE GetHandleModule() const;
	void SetHandleModule(HMODULE value);

	EngineStatus GetEngineStatus() const;
	void SetEngineStatus(EngineStatus value);

	void SignalTick();
	uint64_t WaitForTick(uint64_t lastSeen, uint64_t& droppedOut);
	void WakeTickWaiters();
	uint64_t GetTickGen() const;

	void BeginAISweep();
	void EndAISweep();

	bool WaitForAIIdle(std::chrono::milliseconds timeout);

	void SetTearingDown(bool v);
	bool IsTearingDown() const;

	std::mutex& GetMutex() const;
	std::condition_variable& GetCV() const;

private:
	std::atomic<bool> m_IsRunning{ false };
	std::atomic<HMODULE> m_HandleModule{ nullptr };
	std::atomic<EngineStatus> m_EngineStatus{ EngineStatus::Waiting };

	std::atomic<uint64_t> m_TickGen{ 0 };
	std::mutex m_TickMtx;
	std::condition_variable m_TickCv;

	std::atomic<bool> m_AISweepActive{ false };
	std::atomic<bool> m_EngineTearingDown{ false };

	mutable std::condition_variable m_ShutdownCV{};
	mutable std::mutex m_Mutex{};
};