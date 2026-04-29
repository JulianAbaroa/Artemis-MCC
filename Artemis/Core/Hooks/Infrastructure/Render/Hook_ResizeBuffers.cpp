#include "pch.h"
#include "Core/Threads/Core_Thread.h"
#include "Core/Hooks/Infrastructure/Render/Hook_ResizeBuffers.h"
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Render.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Render.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "External/minhook/include/MinHook.h"

// Manages swap chain resizing to prevent crashes, updating render targets 
// and ensuring video capture is safely halted during resolution changes.
HRESULT __stdcall Hook_ResizeBuffers::HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
	UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	g_pState->Infrastructure->Render->SetResizing(true);

	UINT evenWidth = Width & ~1;
	UINT evenHeight = Height & ~1;
	g_pState->Infrastructure->Render->SetWidth(evenWidth);
	g_pState->Infrastructure->Render->SetHeight(evenHeight);

	if (g_pState->Infrastructure->Render->GetContext())
	{
		g_pState->Infrastructure->Render->GetContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}

	g_pState->Infrastructure->Render->CleanupRTV();

	HRESULT hr = m_OriginalFunction(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	if (SUCCEEDED(hr)) g_pSystem->Infrastructure->Render->Initialize(pSwapChain);

	g_pState->Infrastructure->Render->SetResizing(false);

	return hr;
}

void Hook_ResizeBuffers::Install()
{
	if (m_IsHookInstalled.load()) return;

	auto addresses = g_pSystem->Infrastructure->Render->GetVtableAddresses();
	if (!addresses.ResizeBuffers) return;

	m_FunctionAddress.store(addresses.ResizeBuffers);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedResizeBuffers, reinterpret_cast<LPVOID*>(&m_OriginalFunction)))
	{
		g_pSystem->Debug->Log("[ResizeBuffers] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		g_pSystem->Debug->Log("[ResizeBuffers] ERROR: Failed to enable the hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[ResizeBuffers] INFO: Hook installed.");
}

void Hook_ResizeBuffers::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[ResizeBuffers] INFO: Hook uninstalled.");
}