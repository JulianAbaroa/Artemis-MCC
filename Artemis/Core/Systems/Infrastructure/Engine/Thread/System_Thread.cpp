#include "pch.h"

// Header.
#include "System_Thread.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

// Lifecycle.
#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

bool System_Thread::WaitOrExit(std::chrono::milliseconds ms)
{
	std::unique_lock<std::mutex> lock(g_pState->Infrastructure->Lifecycle->GetMutex());

	bool shutdownTriggered = g_pState->Infrastructure->Lifecycle->GetCV().wait_for(lock, ms, [] {
		return !g_pState->Infrastructure->Lifecycle->IsRunning();
	});

	return !shutdownTriggered;
}