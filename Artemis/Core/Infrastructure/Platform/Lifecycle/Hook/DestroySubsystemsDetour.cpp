module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Lifecycle.Hook;
import :DestroySubsystems;

import Platform.Memory.Type;
import Platform.Lifecycle.Type;
import Platform.Hook.Common;
import std;

namespace
{
	namespace Signature = Platform::Memory::Type::Signature;
	using Status = Platform::Lifecycle::Type::Status;
	using MilliSeconds = std::chrono::milliseconds;
}

namespace Platform::Lifecycle::Hook
{
	DestroySubsystemsDetour* DestroySubsystemsDetour::s_Instance = nullptr;

	auto DestroySubsystemsDetour::IsInProgress() -> bool
	{
		return s_InProgress.load(std::memory_order_acquire);
	}

	auto __fastcall DestroySubsystemsDetour::HookedDestroySubsystems() -> void
	{
		s_InProgress.store(true, std::memory_order_release);

		auto& lifecycle = s_Instance->m_LifecycleStore;
		auto& logs = s_Instance->m_LogsService;

		lifecycle.SetStatus(Status::TearingDown);

		if (!lifecycle.WaitForLoadEnd(MilliSeconds(1000)))
		{
			logs.Message("[DestroySubsystemsDetour] WARNING:"
				" Artemis resources load did not finish within timeout.");
		}
		if (!lifecycle.WaitForTickEnd(MilliSeconds(10)))
		{
			logs.Message("[DestroySubsystemsDetour] WARNING:"
				" Artemis tick did not finish within timeout.");
		}

		s_Instance->m_LifecycleService.RaiseUnhook();
		s_Instance->m_LifecycleService.RaiseCleanup();

		m_OriginalFunction();

		lifecycle.SetStatus(Status::Destroyed);

		logs.Message("[DestroySubsystemsDetour] INFO: Game engine destroyed.");

		s_InProgress.store(false, std::memory_order_release);
	}

	auto DestroySubsystemsDetour::Install() -> bool
	{
		if (m_IsHookInstalled.load()) return true;
		s_Instance = this;

		void* functionAddress = reinterpret_cast<void*>(
			m_AOBService.FindPattern(Signature::DestroySubsystems));
		m_FunctionAddress.store(functionAddress);

		if (!Platform::Hook::Common::InstallDetour(functionAddress,
			reinterpret_cast<void*>(&HookedDestroySubsystems),
			reinterpret_cast<void**>(&m_OriginalFunction),
			"[DestroySubsystemsDetour]", m_LogsService))
		{
			return false;
		}

		m_IsHookInstalled.store(true);
		return true;
	}

	auto DestroySubsystemsDetour::Uninstall() -> void
	{
		if (!m_IsHookInstalled.load()) return;

		Platform::Hook::Common::UninstallDetour(m_FunctionAddress.load(),
			"[DestroySubsystemsDetour]", m_LogsService);

		m_IsHookInstalled.store(false);
	}

	auto DestroySubsystemsDetour::GetFunctionAddress() const -> void*
	{
		return m_FunctionAddress.load();
	}
}