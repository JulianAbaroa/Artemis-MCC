module;

#include "External/minhook/include/MinHook.h"

module Tables.Object.Hook;
import :ReleaseObject;

import Platform.Memory.Type;
import Platform.Hook.Common;

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

    void ReleaseObjectDetour::Install()
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        void* functionAddress = (void*)m_AOBService.FindPattern(Signature::ReleaseObject);
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::Common::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedReleaseObject),
            reinterpret_cast<void**>(&m_OriginalFunction),
            "[ReleaseObjectDetour]", m_LogsService))
        {
            return;
        }

        m_IsHookInstalled.store(true);
    }

    void ReleaseObjectDetour::Uninstall()
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::Common::UninstallDetour(m_FunctionAddress.load(),
            "[ReleaseObjectDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}