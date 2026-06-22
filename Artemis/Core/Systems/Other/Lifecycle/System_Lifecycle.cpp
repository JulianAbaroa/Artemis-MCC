#include "pch.h"

#include "System_Lifecycle.h"

#include "Core/States/Other/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

void System_Lifecycle::SignalShutdown()
{
	m_Deps.State_Lifecycle.SetRunning(false);

	{
		std::lock_guard<std::mutex>	lock(
			m_Deps.State_Lifecycle.GetShutdownMutex());

		m_Deps.State_Lifecycle.GetShutdownCV().notify_all();
	}

	m_Deps.System_Logs.Log("[LifecycleSystem] WARNING:"
		" Shutdown signaled to all threads.");
}