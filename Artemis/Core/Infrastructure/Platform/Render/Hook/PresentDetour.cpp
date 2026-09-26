module;

#include <d3d11.h>
#include <dxgi.h>
#include "External/minhook/include/MinHook.h"

module Platform.Render.Hook;
import :Present;

import Platform.Render.Type;
import Platform.Hook.Common;
import std;

namespace Platform::Render::Hook
{
	PresentDetour* PresentDetour::s_Instance = nullptr;

	auto __stdcall PresentDetour::HookedPresent(IDXGISwapChain* pSwapChain,
		UINT SyncInterval, UINT Flags) -> HRESULT
	{
		auto* self = s_Instance;

		if (self)
		{
			RenderStore::HookScope scope(self->m_RenderStore);

			self->m_TelemetryStore.RecordPresent();
			self->m_RenderService.PresentFrame(pSwapChain);
		}

		return m_OriginalFunction(pSwapChain, SyncInterval, Flags);
	}

    auto PresentDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress = m_SwapChainLocator.Locate().Present;
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::Common::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedPresent),
            reinterpret_cast<void**>(&m_OriginalFunction),
            "[PresentDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto PresentDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::Common::DisableDetour(m_FunctionAddress.load());

        if (!m_RenderService.WaitForIdle(std::chrono::milliseconds(1000)))
        {
            m_LogsService.Message("[PresentDetour] WARNING:"
                " A call was still running when the hook was removed.");
        }

        Platform::Hook::Common::RemoveDetour(m_FunctionAddress.load(),
            "[PresentDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}