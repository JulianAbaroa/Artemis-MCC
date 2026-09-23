module Runtime.Thread;
import :Input;

import std;

namespace Runtime::Thread
{
	auto InputThread::Run() -> void
	{
		auto& lifecycle = m_Platform.m_LifecycleStore;

		m_Service.m_LogsService.Message("[InputThread] INFO: Started.");

		{
			std::unique_lock<std::mutex> lock(lifecycle.GetShutdownMutex());

			lifecycle.GetShutdownCV().wait(lock, [&] {
				return !lifecycle.IsRunning();
			});
		}

		m_Service.m_LogsService.Message("[InputThread] INFO: Stopped.");
	}
}