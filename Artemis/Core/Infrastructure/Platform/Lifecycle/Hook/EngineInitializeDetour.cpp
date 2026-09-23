module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Lifecycle.Hook;
import :EngineInitialize;

import Platform.Memory.Type;
import Platform.Lifecycle.Type;
import std;

namespace
{
	namespace Signature = Platform::Memory::Type::Signature;
	using Status = Platform::Lifecycle::Type::Status;
}

namespace Platform::Lifecycle::Hook
{
	EngineInitializeDetour* EngineInitializeDetour::s_Instance = nullptr;

	auto __fastcall EngineInitializeDetour::HookedEngineInitialize() -> void
	{
		m_OriginalFunction();

		s_Instance->m_LifecycleService.RaiseEngineInitialized();

		s_Instance->m_LifecycleStore.SetStatus(Status::Initialized);

		s_Instance->m_LogsService.Message("[EngineInitializeDetour] INFO:"
			" Game engine initialized.");
	}

	auto EngineInitializeDetour::Install() -> bool
	{
		if (m_IsHookInstalled.load()) return true;
		s_Instance = this;

		void* functionAddress = reinterpret_cast<void*>(
			m_AOBService.FindPattern(Signature::EngineInitialize));

		if (!functionAddress) return false;

		m_FunctionAddress.store(functionAddress);
		MH_RemoveHook(m_FunctionAddress.load());

		if (MH_CreateHook(m_FunctionAddress.load(), &HookedEngineInitialize,
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
		{
			m_LogsService.Message("[EngineInitializeDetour] ERROR:"
				" Failed to create the hook.");
			return false;
		}

		if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
		{
			m_LogsService.Message("[EngineInitializeDetour] ERROR:"
				" Failed to enable the hook.");
			return false;
		}

		m_IsHookInstalled.store(true);
		m_LogsService.Message("[EngineInitializeDetour] INFO: Hook installed.");
		return true;
	}

	auto EngineInitializeDetour::Uninstall() -> void
	{
		if (!m_IsHookInstalled.load()) return;

		MH_DisableHook(m_FunctionAddress.load());
		MH_RemoveHook(m_FunctionAddress.load());

		m_IsHookInstalled.store(false);
		m_LogsService.Message("[EngineInitializeDetour] INFO: Hook uninstalled.");
	}

	auto EngineInitializeDetour::GetFunctionAddress() const -> void*
	{
		return m_FunctionAddress.load();
	}
}