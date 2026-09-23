module;

#include <d3d11.h>
#include <dxgi.h>
#include "External/minhook/include/MinHook.h"

module Platform.Render.Hook;
import :Present;

import Platform.Render.Type;
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
		if (!functionAddress)
		{
			m_LogsService.Message("[PresentDetour] ERROR:"
				" Failed to obtain the function address.");
			return false;
		}

		m_FunctionAddress.store(functionAddress);
		MH_RemoveHook(m_FunctionAddress.load());

		if (MH_CreateHook(m_FunctionAddress.load(), reinterpret_cast<LPVOID>(&HookedPresent),
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
		{
			m_LogsService.Message("[PresentDetour] ERROR:"
				" Failed to create the hook.");
			return false;
		}

		if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
		{
			m_LogsService.Message("[PresentDetour] ERROR:"
				" Failed to enable the hook.");

			MH_RemoveHook(m_FunctionAddress.load());
			return false;
		}

		m_IsHookInstalled.store(true);
		m_LogsService.Message("[PresentDetour] INFO: Hook installed.");
		return true;
	}

	auto PresentDetour::Uninstall() -> void
	{
		if (!m_IsHookInstalled.load()) return;

		MH_DisableHook(m_FunctionAddress.load());

		if (!m_RenderService.WaitForIdle(std::chrono::milliseconds(1000)))
		{
			m_LogsService.Message("[PresentDetour] WARNING:"
				" A call was still running when the hook was removed.");
		}

		MH_RemoveHook(m_FunctionAddress.load());

		m_IsHookInstalled.store(false);
		m_LogsService.Message("[PresentDetour] INFO: Hook uninstalled.");

		s_Instance = nullptr;
	}
}