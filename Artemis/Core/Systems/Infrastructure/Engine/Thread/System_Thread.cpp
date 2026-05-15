#include "pch.h"

// Header.
#include "System_Thread.h"

// --- States ---

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

bool System_Thread::WaitOrExit(std::chrono::milliseconds ms)
{
	std::unique_lock<std::mutex> lock(State_Lifecycle.GetMutex());

	bool shutdownTriggered = State_Lifecycle.GetCV().wait_for(lock, ms, [this] {
		return !State_Lifecycle.IsRunning();
	});

	return !shutdownTriggered;
}