#pragma once

#include <d3d11.h>
#include <atomic>
#pragma comment(lib, "d3d11.lib")

class Hook_GetRawInputData;
class Hook_WndProc;
class State_Render;
class System_Render;
class System_Debug;
class UI_Main;

struct Hook_Present_Depedencies
{
	Hook_GetRawInputData& Hook_GetRawInputData;
	Hook_WndProc& Hook_WndProc;
	State_Render& State_Render;
	System_Render& System_Render;
	System_Debug& System_Debug;
	UI_Main& UI_Main;
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