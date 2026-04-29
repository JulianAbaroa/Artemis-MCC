#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Engine/System_Lifecycle.h"
#include "Core/Systems/Interface/System_Debug.h"

void System_Lifecycle::SignalShutdown()
{
	g_pState->Infrastructure->Lifecycle->SetRunning(false);

	{
		std::lock_guard<std::mutex>	lock(g_pState->Infrastructure->Lifecycle->GetMutex());
		g_pState->Infrastructure->Lifecycle->GetCV().notify_all();
	}

	g_pSystem->Debug->Log("[LifecycleSystem] WARNING: Shutdown signaled to all threads.");
}