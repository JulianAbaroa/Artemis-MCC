module;

#include "External/minhook/include/MinHook.h"

module Tables.Object.Hook;
import :CreateObject;

import Platform.Memory.Type;
import Platform.Hook.Common;

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

    void CreateObjectDetour::Install()
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        void* functionAddress = (void*)m_AOBService.FindPattern(Signature::CreateObject);
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::Common::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedCreateObject),
            reinterpret_cast<void**>(&m_OriginalFunction),
            "[CreateObjectDetour]", m_LogsService))
        {
            return;
        }

        m_IsHookInstalled.store(true);
    }

    void CreateObjectDetour::Uninstall()
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::Common::UninstallDetour(m_FunctionAddress.load(),
            "[CreateObjectDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}