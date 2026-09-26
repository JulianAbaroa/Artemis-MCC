module;

#include <d3d11.h>
#include <dxgi.h>
#include "External/minhook/include/MinHook.h"

module Platform.Render.Hook;
import :ResizeBuffers;

import Platform.Hook.Common;
import std;

namespace Platform::Render::Hook
{
	ResizeBuffersDetour* ResizeBuffersDetour::s_Instance = nullptr;

	auto __stdcall ResizeBuffersDetour::HookedResizeBuffers(IDXGISwapChain* pSwapChain,
		UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
		UINT SwapChainFlags) -> HRESULT
	{
		auto* self = s_Instance;

		if (!self)
		{
			return m_OriginalFunction(pSwapChain, BufferCount,
				Width, Height, NewFormat, SwapChainFlags);
		}

		RenderStore::HookScope scope(self->m_RenderStore);

		self->m_RenderService.BeginResize();

		const HRESULT result = m_OriginalFunction(pSwapChain, BufferCount,
			Width, Height, NewFormat, SwapChainFlags);

		self->m_RenderService.EndResize(pSwapChain, result);

		return result;
	}

    auto ResizeBuffersDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress = m_SwapChainLocator.Locate().ResizeBuffers;
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::Common::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedResizeBuffers),
            reinterpret_cast<void**>(&m_OriginalFunction),
            "[ResizeBuffersDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto ResizeBuffersDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::Common::DisableDetour(m_FunctionAddress.load());

        if (!m_RenderService.WaitForIdle(std::chrono::milliseconds(1000)))
        {
            m_LogsService.Message("[ResizeBuffersDetour] WARNING:"
                " A call was still running when the hook was removed.");
        }

        Platform::Hook::Common::RemoveDetour(m_FunctionAddress.load(),
            "[ResizeBuffersDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}