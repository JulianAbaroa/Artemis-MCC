#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"
#include "Core/Systems/Infrastructure/Engine/System_Thread.h"

bool System_Thread::WaitOrExit(std::chrono::milliseconds ms)
{
	std::unique_lock<std::mutex> lock(g_pState->Infrastructure->Lifecycle->GetMutex());

	bool shutdownTriggered = g_pState->Infrastructure->Lifecycle->GetCV().wait_for(lock, ms, [] {
		return !g_pState->Infrastructure->Lifecycle->IsRunning();
	});

	return !shutdownTriggered;
}