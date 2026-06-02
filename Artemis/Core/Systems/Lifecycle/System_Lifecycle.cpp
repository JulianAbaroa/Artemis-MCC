#include "pch.h"

#include "System_Lifecycle.h"

#include "Core/States/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/Logs/System_Logs.h"

void System_Lifecycle::SignalShutdown()
{
	m_Deps.State_Lifecycle.SetRunning(false);

	{
		std::lock_guard<std::mutex>	lock(
			m_Deps.State_Lifecycle.GetMutex());

		m_Deps.State_Lifecycle.GetCV().notify_all();
	}

	m_Deps.System_Logs.Log("[LifecycleSystem] WARNING:"
		" Shutdown signaled to all threads.");
}