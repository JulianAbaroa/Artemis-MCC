#pragma once

#include <d3d11.h>
#include <atomic>
#pragma comment(lib, "d3d11.lib")

class Hook_GetRawInputData;
class Hook_WndProc;
class State_WorldBuilder;
class State_Tick;
class State_Render;
class State_Telemetry;
class State_Settings;
class System_Render;
class System_MapRenderer;
class System_Logs;
class UI_Launcher;
class UI_Overlay;
class UI_ObjectTable;
class UI_PlayerTable;
class UI_Settings;
class UI_MemoryScanner;
class UI_Logs;

struct Hook_Present_Deps
{
	Hook_GetRawInputData& Hook_GetRawInputData;
	Hook_WndProc& Hook_WndProc;
	State_WorldBuilder& State_WorldBuilder;
	State_Tick& State_Tick;
	State_Render& State_Render;
	State_Telemetry& State_Telemetry;
	State_Settings& State_Settings;
	System_Render& System_Render;
	System_MapRenderer& System_MapRenderer;
	System_Logs& System_Logs;
	UI_Launcher& UI_Launcher;
	UI_Overlay& UI_Overlay;
	UI_ObjectTable& UI_ObjectTable;
	UI_PlayerTable& UI_PlayerTable;
	UI_Settings& UI_Settings;
	UI_MemoryScanner& UI_MemoryScanner;
	UI_Logs& UI_Logs;
};

class Hook_Present
{
public:
	Hook_Present(Hook_Present_Deps deps) : m_Deps(deps) {}
	~Hook_Present() = default;

	void Install();
	void Uninstall();
	
private:
	static Hook_Present* s_Instance;
	Hook_Present_Deps m_Deps;

	static HRESULT __stdcall HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

	typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

	static inline Present_t m_OriginalPresent{ nullptr };
	std::atomic<bool> m_PresentHookInstalled{ false };
	void* m_PresentAddress{ nullptr };
};