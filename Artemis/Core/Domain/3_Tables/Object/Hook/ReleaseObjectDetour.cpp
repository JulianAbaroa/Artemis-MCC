module;

#include "External/minhook/include/MinHook.h"

module Tables.Object.Hook;
import :ReleaseObject;

import Platform.Memory.Type;

namespace
{
	namespace Signature = Platform::Memory::Type::Signature;
}

namespace Tables::Object::Hook
{
	auto __fastcall ReleaseObjectDetour::HookedReleaseObject(
		unsigned int handle) -> void
	{
		s_Instance->m_ObjectService.OnObjectDestroyed(handle);
		m_OriginalFunction(handle);
	}

	ReleaseObjectDetour* ReleaseObjectDetour::s_Instance = nullptr;

	auto ReleaseObjectDetour::Install() -> void
	{
		if (m_IsHookInstalled.load()) return;
		s_Instance = this;

		void* functionAddress = (void*)s_Instance->m_AOBService.FindPattern(
			Signature::ReleaseObject);

		if (!functionAddress)
		{
			s_Instance->m_LogsService.Message("[ReleaseObjectDetour] ERROR:"
				" Failed to obtain the function address.");
			return;
		}

		m_FunctionAddress.store(functionAddress);
		if (MH_CreateHook(m_FunctionAddress.load(),
			&this->HookedReleaseObject,
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
		{
			s_Instance->m_LogsService.Message("[ReleaseObjectDetour] ERROR:"
				" Failed to create the hook.");
			return;
		}
		if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
		{
			s_Instance->m_LogsService.Message("[ReleaseObjectDetour] ERROR:"
				" Failed to enable hook.");
			return;
		}

		m_IsHookInstalled.store(true);
		s_Instance->m_LogsService.Message("[ReleaseObjectDetour] INFO:"
			" Hook installed.");
		return;
	}

	auto ReleaseObjectDetour::Uninstall() -> void
	{
		if (!m_IsHookInstalled.load()) return;

		MH_DisableHook(m_FunctionAddress.load());
		MH_RemoveHook(m_FunctionAddress.load());

		m_IsHookInstalled.store(false);

		s_Instance->m_LogsService.Message("[ReleaseObjectDetour] INFO:"
			" Hook uninstalled.");

		s_Instance = nullptr;
	}
}