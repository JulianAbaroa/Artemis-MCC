#include "pch.h"

#include "Hook_ResizeBuffers.h"

#include "Core/States/Render/State_Render.h"

#include "Core/Systems/Render/System_Render.h"
#include "Core/Systems/Logs/System_Logs.h"

#include "External/minhook/include/MinHook.h"

// Manages swap chain resizing to prevent crashes, updating render targets 
// and ensuring video capture is safely halted during resolution changes.
HRESULT __stdcall Hook_ResizeBuffers::HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
	UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	s_Instance->m_Deps.State_Render.SetResizing(true);

	UINT evenWidth = Width & ~1;
	UINT evenHeight = Height & ~1;
	s_Instance->m_Deps.State_Render.SetWidth(evenWidth);
	s_Instance->m_Deps.State_Render.SetHeight(evenHeight);

	if (s_Instance->m_Deps.State_Render.GetContext())
	{
		s_Instance->m_Deps.State_Render.GetContext()
			->OMSetRenderTargets(0, nullptr, nullptr);
	}

	s_Instance->m_Deps.State_Render.CleanupRTV();

	HRESULT hr = m_OriginalFunction(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	if (SUCCEEDED(hr)) s_Instance->m_Deps.System_Render.Initialize(pSwapChain);

	s_Instance->m_Deps.State_Render.SetResizing(false);

	return hr;
}

Hook_ResizeBuffers* Hook_ResizeBuffers::s_Instance = nullptr;

void Hook_ResizeBuffers::Install()
{
	if (m_IsHookInstalled.load()) return;
	s_Instance = this;

	auto addresses = s_Instance->m_Deps.System_Render.GetVtableAddresses();
	if (!addresses.ResizeBuffers) return;

	m_FunctionAddress.store(addresses.ResizeBuffers);
	if (MH_CreateHook(
			m_FunctionAddress.load(), 
			&this->HookedResizeBuffers, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) 
		!= MH_OK)
	{
		s_Instance->m_Deps.System_Logs.Log("[ResizeBuffers] ERROR:"
			" Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		s_Instance->m_Deps.System_Logs.Log("[ResizeBuffers] ERROR:"
			" Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Logs.Log("[ResizeBuffers] INFO:"
		" Hook installed.");
}

void Hook_ResizeBuffers::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Logs.Log("[ResizeBuffers] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}