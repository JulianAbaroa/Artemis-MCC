#pragma once

#include <winuser.h>
#include <atomic>

class State_Settings;
class System_Logs;
class UI_Launcher;

struct Hook_GetRawInputData_Dependencies
{
	State_Settings& State_Settings;
	System_Logs& System_Logs;
	UI_Launcher& UI_Launcher;
};

class Hook_GetRawInputData
{
public:
	Hook_GetRawInputData(Hook_GetRawInputData_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_GetRawInputData() = default;

	void Install();
	void Uninstall();

private:
	static Hook_GetRawInputData* s_Instance;
	Hook_GetRawInputData_Dependencies m_Deps;

	static UINT WINAPI HookedGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData,
		PUINT pcbSize, UINT cbSizeHeader);

	typedef UINT(WINAPI* GetRawInputData_t)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);

	static inline GetRawInputData_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};