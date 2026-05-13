#include "pch.h"

// Header.
#include "Hook_WndProc.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/States/Infrastructure/Engine/Memory/State_MemoryScanner.h"
#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"

// --- Systems ---
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_MemoryScanner.h"
#include "Core/Systems/Infrastructure/Engine/Lifecycle/System_Lifecycle.h"
#include "Core/Systems/Infrastructure/Persistence/System_Preferences.h"

#include "Core/Systems/Interface/System_Debug.h"

// ImGui.
#include "External/imgui/imgui.h"

#include <chrono>

using namespace std::chrono_literals;

// Intercepts the window's message procedure to filter input events,
// granting ImGui priority and handling system-level lifecycle events.
LRESULT __stdcall Hook_WndProc::HookedWndProc(
	const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	auto& debug = *g_pSystem->Debug;
	auto& stateLifecycle = *g_pState->Infrastructure->Lifecycle;
	auto& systemLifecycle = *g_pSystem->Infrastructure->Lifecycle;
	auto& preferences = *g_pSystem->Infrastructure->Preferences;
	auto& settings = *g_pState->Infrastructure->Settings;

	// WM_CLOSE/WM_DESTROY/WM_QUIT: 
	// Intercept the window closing to clear the state.
	bool windowDestroyed = 
		(uMsg == WM_CLOSE || uMsg == WM_DESTROY || uMsg == WM_QUIT);

	if (windowDestroyed)
	{
		if (stateLifecycle.IsRunning())
		{
			debug.Log("[WndProc] WARNING: MCC shutdown detected.");
			preferences.SavePreferences();
			systemLifecycle.SignalShutdown();
		}
		
		return CallWindowProc(m_OriginalWndProc, hWnd, uMsg, wParam, lParam);
	}

	// WM_SYSKEYDOWN + VK_F4: 
	// Ignore Alt+F4 so that Windows handles it by default.
	bool altF4Pressed = (uMsg == WM_SYSKEYDOWN && wParam == VK_F4);
	if (altF4Pressed)
	{
		return CallWindowProc(m_OriginalWndProc, hWnd, uMsg, wParam, lParam);
	}

	// WM_KEYDOWN/WM_SYSKEYDOWN: 
	// It captures heartbeats before they reach the motor.
	bool isKeyDown = (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN);
	if (isKeyDown) if (HandleHotKeys(wParam)) return 0;

	// If the menu is visible, ImGui has input priority.
	if (settings.IsMenuVisible())
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		{
			return 1;
		}

		// WM_KEYFIRST/WM_KEYLAST: 
		// Range that covers from KeyDown to KeyUp.
		bool wantCaptureKeyboard = ImGui::GetIO().WantCaptureKeyboard;
		bool isKeyboardMessage = (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST);
		if (wantCaptureKeyboard && isKeyboardMessage) return 0;

		// Filter clicks and scrolling if the mouse is over an ImGui window.
		bool wantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
		if (wantCaptureMouse)
		{
			switch (uMsg)
			{
			case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
			case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
				return 0;
			}
		}
	}
	
	return CallWindowProc(m_OriginalWndProc, hWnd, uMsg, wParam, lParam);
}

// Private helper to handle mod-specific hotkeys.
bool Hook_WndProc::HandleHotKeys(WPARAM wParam)
{
	auto& settings = *g_pState->Infrastructure->Settings;
	auto& stateMemoryScanner = *g_pState->Infrastructure->MemoryScanner;
	auto& systemMemoryScanner = *g_pSystem->Infrastructure->MemoryScanner;
	
	bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

	if (shiftPressed && wParam == '1')
	{
		settings.SetMenuVisible(!settings.IsMenuVisible());
		return true;
	}

	if (shiftPressed && wParam == '2')
	{
		settings.SetMenuVisible(true);
		settings.SetForceMenuReset(true);
		return true;
	}

	if (shiftPressed && wParam == '3')
	{
		bool currentLock = settings.IsMenuLocked();
		settings.SetMenuLocked(!currentLock);
		return true;
	}

	if (shiftPressed && wParam == '4')
	{
		if (!g_pState->Infrastructure->MemoryScanner->IsScanning())
		{
			systemMemoryScanner.TriggerScan(
				stateMemoryScanner.GetDelayMs());
		}

		return true;
	}

	return false;
}

WNDPROC Hook_WndProc::GetWndProc()
{
	return m_OriginalWndProc;
}

void Hook_WndProc::SetWndProc(WNDPROC lpPrevWndFunc)
{
	m_OriginalWndProc = lpPrevWndFunc;
}