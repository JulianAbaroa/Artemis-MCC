#pragma once

#include <d3d11.h>
#include <atomic>
#pragma comment(lib, "d3d11.lib")

class Hook_GetRawInputData;
class Hook_WndProc;
class State_Render;
class System_Render;
class System_Logs;
class UI_Launcher;
class UI_Overlay;
class UI_ObjectTable;
class UI_PlayerTable;
class UI_Map;
class UI_Settings;
class UI_MemoryScanner;
class UI_Logs;

struct Hook_Present_Depedencies
{
	Hook_GetRawInputData& Hook_GetRawInputData;
	Hook_WndProc& Hook_WndProc;
	State_Render& State_Render;
	System_Render& System_Render;
	System_Logs& System_Logs;
	UI_Launcher& UI_Launcher;
	UI_Overlay& UI_Overlay;
	UI_ObjectTable& UI_ObjectTable;
	UI_PlayerTable& UI_PlayerTable;
	UI_Map& UI_Map;
	UI_Settings& UI_Settings;
	UI_MemoryScanner& UI_MemoryScanner;
	UI_Logs& UI_Logs;
};

class Hook_Present
{
public:
	Hook_Present(Hook_Present_Depedencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_Present() = default;

	void Install();
	void Uninstall();
	
private:
	static Hook_Present* s_Instance;
	Hook_Present_Depedencies m_Deps;

	static HRESULT __stdcall HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

	typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

	static inline Present_t m_OriginalPresent{ nullptr };
	std::atomic<bool> m_PresentHookInstalled{ false };
	void* m_PresentAddress{ nullptr };
};