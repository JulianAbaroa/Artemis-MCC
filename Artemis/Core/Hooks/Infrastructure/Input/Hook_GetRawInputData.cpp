#include "pch.h"

// Header.
#include "Hook_GetRawInputData.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/States/Infrastructure/Persistence/State_Settings.h"

// --- Systems ---
#include "Core/Systems/Core_System.h"

#include "Core/Systems/Interface/System_Debug.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

// Intercepts Windows Raw Input messages to prevent the game from 
// processing mouse movement and clicks when the ImGui menu is active.
UINT WINAPI Hook_GetRawInputData::HookedGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, 
	LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) 
{
	UINT dwSize = m_OriginalFunction(
		hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

	auto& settings = *g_pState->Infrastructure->Settings;
	if (dwSize != (UINT)-1 && pData != NULL && 
		settings.IsMenuVisible() && settings.ShouldFreezeMouse())
	{
		RAWINPUT* raw = (RAWINPUT*)pData;
		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			raw->data.mouse.lLastX = 0;
			raw->data.mouse.lLastY = 0;

			raw->data.mouse.ulButtons = 0;
			raw->data.mouse.usButtonFlags = 0;
		}
	}

	return dwSize;
}

void Hook_GetRawInputData::Install()
{
	if (m_IsHookInstalled.load()) return;

	auto& debug = *g_pSystem->Debug;

	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	if (!hUser32) 
	{
		debug.Log("[GetRawInputData] ERROR: Could not get"
			" handle for user32.dll");
		return;
	}

	m_FunctionAddress.store((void*)GetProcAddress(hUser32, "GetRawInputData"));
	if (!m_FunctionAddress.load()) 
	{
		debug.Log("[GetRawInputData] ERROR: GetProcAddress for"
			" GetRawInputData failed");
		return;
	}

	if (MH_CreateHook(
			m_FunctionAddress.load(), 
			&this->HookedGetRawInputData, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) 
		!= MH_OK)
	{
		debug.Log("[GetRawInputData] ERROR: Failed to create the hook.");
	}

	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		debug.Log("[GetRawInputData] ERROR: Failed to enable the hook.");
		return;
	}
	
	m_IsHookInstalled.store(true);
	debug.Log("[GetRawInputData] INFO: Hook installed.");
}

void Hook_GetRawInputData::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	auto& debug = *g_pSystem->Debug;
	debug.Log("[GetRawInputData] INFO: Hook uninstalled.");
}