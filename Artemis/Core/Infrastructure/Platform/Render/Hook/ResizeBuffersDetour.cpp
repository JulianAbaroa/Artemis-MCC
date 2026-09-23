module;

#include <d3d11.h>
#include <dxgi.h>
#include "External/minhook/include/MinHook.h"

module Platform.Render.Hook;
import :ResizeBuffers;

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
		if (!functionAddress)
		{
			m_LogsService.Message("[ResizeBuffersDetour] ERROR:"
				" Failed to obtain the function address.");
			return false;
		}

		m_FunctionAddress.store(functionAddress);
		MH_RemoveHook(m_FunctionAddress.load());

		if (MH_CreateHook(m_FunctionAddress.load(), reinterpret_cast<LPVOID>(&HookedResizeBuffers),
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
		{
			m_LogsService.Message("[ResizeBuffersDetour] ERROR:"
				" Failed to create the hook.");
			return false;
		}

		if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
		{
			m_LogsService.Message("[ResizeBuffersDetour] ERROR:"
				" Failed to enable the hook.");

			MH_RemoveHook(m_FunctionAddress.load());
			return false;
		}

		m_IsHookInstalled.store(true);
		m_LogsService.Message("[ResizeBuffersDetour] INFO: Hook installed.");
		return true;
	}

	auto ResizeBuffersDetour::Uninstall() -> void
	{
		if (!m_IsHookInstalled.load()) return;

		MH_DisableHook(m_FunctionAddress.load());

		if (!m_RenderService.WaitForIdle(std::chrono::milliseconds(1000)))
		{
			m_LogsService.Message("[ResizeBuffersDetour] WARNING:"
				" A call was still running when the hook was removed.");
		}

		MH_RemoveHook(m_FunctionAddress.load());

		m_IsHookInstalled.store(false);
		m_LogsService.Message("[ResizeBuffersDetour] INFO: Hook uninstalled.");

		s_Instance = nullptr;
	}
}