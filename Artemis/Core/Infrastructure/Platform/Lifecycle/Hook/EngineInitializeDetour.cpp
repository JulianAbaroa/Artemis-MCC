module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Lifecycle.Hook;
import :EngineInitialize;

import Platform.Memory.Type;
import Platform.Lifecycle.Type;
import Platform.Hook.Common;
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
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::Common::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedEngineInitialize),
            reinterpret_cast<void**>(&m_OriginalFunction),
            "[EngineInitializeDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto EngineInitializeDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::Common::UninstallDetour(m_FunctionAddress.load(),
            "[EngineInitializeDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
    }

	auto EngineInitializeDetour::GetFunctionAddress() const -> void*
	{
		return m_FunctionAddress.load();
	}
}