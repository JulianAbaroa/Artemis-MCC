module;

#include "External/minhook/include/MinHook.h"

module Tables.Object.Hook;
import :CreateObject;

import Platform.Memory.Type;

namespace
{
	namespace Signature = Platform::Memory::Type::Signature;
}

namespace Tables::Object::Hook
{
	auto __fastcall CreateObjectDetour::HookedCreateObject(
		unsigned short* placementData) -> unsigned long long
	{
		auto handle = m_OriginalFunction(placementData);

		std::uint32_t datumIndex =
			*(std::uint32_t*)((unsigned char*)placementData + 0x00);

		s_Instance->m_ObjectService.OnObjectCreated(
			static_cast<std::uint32_t>(handle), datumIndex);

		return handle;
	}

	CreateObjectDetour* CreateObjectDetour::s_Instance = nullptr;

	auto CreateObjectDetour::Install() -> void
	{
		if (m_IsHookInstalled.load()) return;
		s_Instance = this;

		void* functionAddress = (void*)m_AOBService.FindPattern(
			Signature::CreateObject);

		if (!functionAddress)
		{
			s_Instance->m_LogsService.Message("[CreateObjectDetour] ERROR:"
				" Failed to obtain the function address.");
			return;
		}

		m_FunctionAddress.store(functionAddress);
		if (MH_CreateHook(m_FunctionAddress.load(),
			&this->HookedCreateObject,
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
		{
			s_Instance->m_LogsService.Message("[CreateObjectDetour] ERROR:"
				" Failed to create the hook.");
			return;
		}
		if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
		{
			s_Instance->m_LogsService.Message("[CreateObjectDetour] ERROR:"
				" Failed to enable hook.");
			return;
		}

		m_IsHookInstalled.store(true);
		s_Instance->m_LogsService.Message("[CreateObjectDetour] INFO:"
			" Hook installed.");
		return;
	}

	auto CreateObjectDetour::Uninstall() -> void
	{
		if (!m_IsHookInstalled.load()) return;

		MH_DisableHook(m_FunctionAddress.load());
		MH_RemoveHook(m_FunctionAddress.load());

		m_IsHookInstalled.store(false);

		s_Instance->m_LogsService.Message("[CreateObjectDetour] INFO:"
			" Hook uninstalled.");

		s_Instance = nullptr;
	}
}