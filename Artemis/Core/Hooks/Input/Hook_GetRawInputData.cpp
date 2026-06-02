#include "pch.h"

#include "Hook_GetRawInputData.h"

#include "Core/States/Settings/State_Settings.h"

#include "Core/Systems/Logs/System_Logs.h"

#include "Core/UI/Launcher/UI_Launcher.h"
#include "Core/UI/Launcher/UI_Tab.h"

#include "External/minhook/include/MinHook.h"

UINT WINAPI Hook_GetRawInputData::HookedGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand,
	LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)
{
	UINT dwSize = m_OriginalFunction(
		hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

	bool isValid = dwSize != (UINT)-1 && pData != NULL;

	bool shouldFreezeMouse =
		s_Instance->m_Deps.State_Settings.ShouldFreezeMouse();

	bool anyVisible = false;
	for (auto& tab : s_Instance->m_Deps.UI_Launcher.GetTabs())
	{
		if (tab->IsVisible()) 
		{ 
			anyVisible = true; 
			break; 
		}
	}
	
	bool launcherVisible = 
		s_Instance->m_Deps.UI_Launcher.s_IsVisible;

	if (isValid && shouldFreezeMouse && 
		(anyVisible || launcherVisible))
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

Hook_GetRawInputData* Hook_GetRawInputData::s_Instance = nullptr;

void Hook_GetRawInputData::Install()
{
	if (m_IsHookInstalled.load()) return;
	s_Instance = this;

	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	if (!hUser32) 
	{
		s_Instance->m_Deps.System_Logs.Log("[GetRawInputData] ERROR:"
			" Could not get handle for user32.dll");
		return;
	}

	m_FunctionAddress.store((void*)GetProcAddress(hUser32, "GetRawInputData"));
	if (!m_FunctionAddress.load()) 
	{
		s_Instance->m_Deps.System_Logs.Log("[GetRawInputData] ERROR:"
			" GetProcAddress for GetRawInputData failed");
		return;
	}

	if (MH_CreateHook(
			m_FunctionAddress.load(), 
			&this->HookedGetRawInputData, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)) 
		!= MH_OK)
	{
		s_Instance->m_Deps.System_Logs.Log("[GetRawInputData] ERROR:"
			" Failed to create the hook.");
	}

	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		s_Instance->m_Deps.System_Logs.Log("[GetRawInputData] ERROR:"
			" Failed to enable the hook.");
		return;
	}
	
	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Logs.Log("[GetRawInputData] INFO:"
		" Hook installed.");
}

void Hook_GetRawInputData::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Logs.Log("[GetRawInputData] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}