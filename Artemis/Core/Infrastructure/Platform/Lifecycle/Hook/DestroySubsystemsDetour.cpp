module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Lifecycle.Hook;
import :DestroySubsystems;

import Platform.Memory.Type;
import Platform.Lifecycle.Type;
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

		if (!functionAddress) return false;

		m_FunctionAddress.store(functionAddress);
		MH_RemoveHook(m_FunctionAddress.load());

		if (MH_CreateHook(m_FunctionAddress.load(), &HookedDestroySubsystems,
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
		{
			m_LogsService.Message("[DestroySubsystemsDetour] ERROR:"
				" Failed to create the hook.");
			return false;
		}

		if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
		{
			m_LogsService.Message("[DestroySubsystemsDetour] ERROR:"
				" Failed to enable the hook.");
			return false;
		}

		m_IsHookInstalled.store(true);
		m_LogsService.Message("[DestroySubsystemsDetour] INFO: Hook installed.");
		return true;
	}

	auto DestroySubsystemsDetour::Uninstall() -> void
	{
		if (!m_IsHookInstalled.load()) return;

		MH_DisableHook(m_FunctionAddress.load());
		MH_RemoveHook(m_FunctionAddress.load());

		m_IsHookInstalled.store(false);
		m_LogsService.Message("[DestroySubsystemsDetour] INFO: Hook uninstalled.");
	}

	auto DestroySubsystemsDetour::GetFunctionAddress() const -> void*
	{
		return m_FunctionAddress.load();
	}
}