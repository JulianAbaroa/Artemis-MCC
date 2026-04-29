#pragma once

#include "Core/Types/Domain/ModTypes.h"
#include <condition_variable>
#include <atomic>

class State_Lifecycle
{
public:
	bool IsRunning() const;
	void SetRunning(bool value);

	HMODULE GetHandleModule() const;
	void SetHandleModule(HMODULE value);

	EngineStatus GetEngineStatus() const;
	void SetEngineStatus(EngineStatus value);

	std::mutex& GetMutex() const;
	std::condition_variable& GetCV() const;

private:
	std::atomic<bool> m_IsRunning{ false };
	std::atomic<HMODULE> m_HandleModule{ nullptr };
	std::atomic<EngineStatus> m_EngineStatus{ EngineStatus::Waiting };

	mutable std::condition_variable m_ShutdownCV{};
	mutable std::mutex m_Mutex{};
};