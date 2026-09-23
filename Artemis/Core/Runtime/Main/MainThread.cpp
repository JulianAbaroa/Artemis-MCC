module;

#include <windows.h>

module Runtime.Thread;
import :Main;

import Platform.Lifecycle.Hook;
import Platform.Lifecycle.Type;
import Platform.Input.Hook;
import Platform.Render.Hook;
import std;

using namespace std::chrono_literals;

namespace
{
	using Status = Platform::Lifecycle::Type::Status;
	using DestroySubsystemsDetour = Platform::Lifecycle::Hook::DestroySubsystemsDetour;
}

namespace Runtime::Thread
{
	auto MainThread::Run() -> void
	{
		auto& lifecycle = m_Platform.m_LifecycleStore;
		auto& logs = m_Service.m_LogsService;

		this->WaitOrExit(5000ms);

		logs.Message("[MainThread] INFO: Started.");

		if (!this->InstallLifecycleHooks()) this->Shutdown();

		if (lifecycle.IsRunning()) this->InstallRenderHooks();

		while (lifecycle.IsRunning())
		{
			this->CheckHooksHealth();
			m_Service.m_TelemetryService.Update();

			if (lifecycle.GetStatus() == Status::Destroyed)
			{
				logs.Message("[MainThread] INFO: Game engine destruction detected.");

				while (DestroySubsystemsDetour::IsInProgress())
				{
					std::this_thread::sleep_for(5ms);
				}

				if (!this->IsStillRunning()) break;

				m_Platform.m_EngineInitializeDetour.Uninstall();
				m_Platform.m_DestroySubsystemsDetour.Uninstall();

				if (!this->IsStillRunning()) break;

				if (!this->InstallLifecycleHooks())
				{
					logs.Message("[MainThread] ERROR:"
						" Failed to re-install hooks after engine reset.");

					this->Shutdown();
					return;
				}

				lifecycle.SetStatus(Status::Waiting);
			}

			this->WaitOrExit(1000ms);
		}

		m_Platform.m_LifecycleService.RaiseShutdown();

		m_Platform.m_EngineInitializeDetour.Uninstall();
		m_Platform.m_DestroySubsystemsDetour.Uninstall();

		logs.Message("[MainThread] INFO: Stopped.");
	}

	auto MainThread::WaitOrExit(std::chrono::milliseconds ms) -> bool
	{
		auto& lifecycle = m_Platform.m_LifecycleStore;

		std::unique_lock<std::mutex> lock(lifecycle.GetShutdownMutex());

		bool shouldExit = lifecycle.GetShutdownCV().wait_for(lock, ms, [&] {
			return !lifecycle.IsRunning();
			});

		return !shouldExit;
	}

	auto MainThread::InstallLifecycleHooks() -> bool
	{
		while (m_Platform.m_LifecycleStore.IsRunning())
		{
			if (m_Platform.m_EngineInitializeDetour.Install() &&
				m_Platform.m_DestroySubsystemsDetour.Install())
			{
				return true;
			}

			this->WaitOrExit(1000ms);
		}

		return false;
	}

	auto MainThread::InstallRenderHooks() -> bool
	{
		constexpr int k_MaxAttempts = 5;

		for (int attempt = 0; attempt < k_MaxAttempts &&
			m_Platform.m_LifecycleStore.IsRunning(); ++attempt)
		{
			if (m_Platform.m_PresentDetour.Install() &&
				m_Platform.m_ResizeBuffersDetour.Install())
			{
				m_Platform.m_GetRawInputDataDetour.Install();
				return true;
			}

			this->WaitOrExit(1000ms);
		}

		m_Service.m_LogsService.Message("[MainThread] ERROR:"
			" Failed to install the render hooks. Artemis keeps running without them.");

		return false;
	}

	auto MainThread::Shutdown() -> void
	{
		m_Service.m_LogsService.Message("[MainThread] ERROR:"
			" Initiating emergency shutdown.");

		m_Platform.m_LifecycleService.SignalShutdown();
	}

	auto MainThread::CheckHooksHealth() -> void
	{
		auto& lifecycle = m_Platform.m_LifecycleStore;

		if (lifecycle.GetStatus() == Status::Destroyed) return;
		if (DestroySubsystemsDetour::IsInProgress()) return;

		void* engineInitAddr = m_Platform.m_EngineInitializeDetour.GetFunctionAddress();
		void* detroyAddr = m_Platform.m_DestroySubsystemsDetour.GetFunctionAddress();

		bool areHooksCorrupted =
			!this->IsHookIntact(engineInitAddr) ||
			!this->IsHookIntact(detroyAddr);

		if (areHooksCorrupted && lifecycle.IsRunning())
		{
			m_Service.m_LogsService.Message("[MainThread] WARNING:"
				" Hooks corrupted, rebooting.");

			lifecycle.SetStatus(Status::Destroyed);
		}
	}

	auto MainThread::IsHookIntact(void* address) -> bool
	{
		if (address == nullptr) return false;

		unsigned char firstByte{};
		SIZE_T bytesRead{};

		if (ReadProcessMemory(GetCurrentProcess(), address,
			&firstByte, 1, &bytesRead))
		{
			return firstByte == 0xE9;
		}

		return false;
	}

	auto MainThread::IsStillRunning() -> bool
	{
		this->WaitOrExit(1000ms);

		return m_Platform.m_LifecycleStore.IsRunning();
	}
}