#pragma once

#include "External/imgui/backends/imgui_impl_win32.h"

#include <Windows.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class State_MemoryScanner;
class State_Lifecycle;
class State_Settings;
class State_FlyCamera;
class State_Selection;
class State_OverlayMode;
class System_MemoryScanner;
class System_Lifecycle;
class System_Logs;
class UI_Launcher;
class UI_Overlay;

struct Hook_WndProc_Dependencies
{
	State_MemoryScanner& State_MemoryScanner;
	State_Lifecycle& State_Lifecycle;
	State_Settings& State_Settings;
	State_FlyCamera& State_FlyCamera;
	State_Selection& State_Selection;
	State_OverlayMode& State_OverlayMode;
	System_MemoryScanner& System_MemoryScanner;
	System_Lifecycle& System_Lifecycle;
	System_Logs& System_Logs;
	UI_Launcher& UI_Launcher;
	UI_Overlay& UI_Overlay;
};

class Hook_WndProc
{
public:
	Hook_WndProc(Hook_WndProc_Dependencies dependencies) :
		m_Deps(dependencies) { s_Instance = this; }
	~Hook_WndProc() { s_Instance = nullptr; };

	void Install(HWND hwnd);
	void Uninstall();

	static LRESULT __stdcall HookedWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	static WNDPROC GetWndProc();
	static void SetWndProc(WNDPROC lpPrevWndFunc);

private:
	static Hook_WndProc* s_Instance;
	Hook_WndProc_Dependencies m_Deps;

	static bool HandleHotKeys(WPARAM wParam);

	static inline WNDPROC m_OriginalWndProc = nullptr;
	static inline HWND m_hWnd = nullptr;
};