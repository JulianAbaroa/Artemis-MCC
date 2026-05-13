#include "pch.h"

// Header.
#include "Hook_ResizeBuffers.h"

// --- States ---

#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/States/Infrastructure/Engine/Render/State_Render.h"

// --- Systems ---

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Core/Systems/Infrastructure/Engine/Render/System_Render.h"

#include "Core/Systems/Interface/System_Debug.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

// Manages swap chain resizing to prevent crashes, updating render targets 
// and ensuring video capture is safely halted during resolution changes.
HRESULT __stdcall Hook_ResizeBuffers::HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
	UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	auto& stateRender = *g_pState->Infrastructure->Render;
	auto& systemRender = *g_pSystem->Infrastructure->Render;

	stateRender.SetResizing(true);

	UINT evenWidth = Width & ~1;
	UINT evenHeight = Height & ~1;
	stateRender.SetWidth(evenWidth);
	stateRender.SetHeight(evenHeight);

	if (stateRender.GetContext())
	{
		stateRender.GetContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}

	stateRender.CleanupRTV();

	HRESULT hr = m_OriginalFunction(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	if (SUCCEEDED(hr)) systemRender.Initialize(pSwapChain);

	stateRender.SetResizing(false);

	return hr;
}

void Hook_ResizeBuffers::Install()
{
	if (m_IsHookInstalled.load()) return;

	auto& debug = *g_pSystem->Debug;
	auto& render = *g_pSystem->Infrastructure->Render;

	auto addresses = render.GetVtableAddresses();
	if (!addresses.ResizeBuffers) return;

	m_FunctionAddress.store(addresses.ResizeBuffers);
	if (MH_CreateHook(
			m_FunctionAddress.load(), 
			&this->HookedResizeBuffers, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) 
		!= MH_OK)
	{
		debug.Log("[ResizeBuffers] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		debug.Log("[ResizeBuffers] ERROR: Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	debug.Log("[ResizeBuffers] INFO: Hook installed.");
}

void Hook_ResizeBuffers::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	auto& debug = *g_pSystem->Debug;
	debug.Log("[ResizeBuffers] INFO: Hook uninstalled.");
}