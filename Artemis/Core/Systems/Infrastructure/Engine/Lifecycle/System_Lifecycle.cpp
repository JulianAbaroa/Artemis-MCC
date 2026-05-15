#include "pch.h"

// Header.
#include "System_Lifecycle.h"

// --- States ---

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

void System_Lifecycle::SignalShutdown()
{
	m_Deps.State_Lifecycle.SetRunning(false);

	{
		std::lock_guard<std::mutex>	lock(
			m_Deps.State_Lifecycle.GetMutex());

		m_Deps.State_Lifecycle.GetCV().notify_all();
	}

	m_Deps.System_Debug.Log("[LifecycleSystem] WARNING:"
		" Shutdown signaled to all threads.");
}