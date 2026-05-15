#include "pch.h"

// Header.
#include "Hook_WndProc.h"

// --- States ---

#include "Core/States/Infrastructure/Engine/Memory/State_MemoryScanner.h"
#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"

// --- Systems ---

#include "Core/Systems/Infrastructure/Engine/Memory/System_MemoryScanner.h"
#include "Core/Systems/Infrastructure/Engine/Lifecycle/System_Lifecycle.h"
#include "Core/Systems/Infrastructure/Persistence/System_Preferences.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

// ImGui.
#include "External/imgui/imgui.h"

#include <chrono>

using namespace std::chrono_literals;

// Intercepts the window's message procedure to filter input events,
// granting ImGui priority and handling system-level lifecycle events.
LRESULT __stdcall Hook_WndProc::HookedWndProc(
	const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	if (s_Instance == nullptr)
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	// WM_CLOSE/WM_DESTROY/WM_QUIT: 
	// Intercept the window closing to clear the state.
	bool windowDestroyed = 
		(uMsg == WM_CLOSE || uMsg == WM_DESTROY || uMsg == WM_QUIT);

	if (windowDestroyed)
	{
		if (s_Instance->m_Deps.State_Lifecycle.IsRunning())
		{
			s_Instance->m_Deps.System_Debug.Log("[WndProc] WARNING:"
				" MCC shutdown detected.");

			s_Instance->m_Deps.System_Preferences.SavePreferences();
			s_Instance->m_Deps.System_Lifecycle.SignalShutdown();
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
	if (s_Instance->m_Deps.State_Settings.IsMenuVisible())
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
	bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

	if (shiftPressed && wParam == '1')
	{
		s_Instance->m_Deps.State_Settings.SetMenuVisible(
			!s_Instance->m_Deps.State_Settings.IsMenuVisible());
		return true;
	}

	if (shiftPressed && wParam == '2')
	{
		s_Instance->m_Deps.State_Settings.SetMenuVisible(true);
		s_Instance->m_Deps.State_Settings.SetForceMenuReset(true);
		return true;
	}

	if (shiftPressed && wParam == '3')
	{
		bool currentLock = s_Instance->m_Deps.State_Settings.IsMenuLocked();
		s_Instance->m_Deps.State_Settings.SetMenuLocked(!currentLock);
		return true;
	}

	if (shiftPressed && wParam == '4')
	{
		if (!s_Instance->m_Deps.State_MemoryScanner.IsScanning())
		{
			s_Instance->m_Deps.System_MemoryScanner.TriggerScan(
				s_Instance->m_Deps.State_MemoryScanner.GetDelayMs());
		}

		return true;
	}

	return false;
}

Hook_WndProc* Hook_WndProc::s_Instance = nullptr;

WNDPROC Hook_WndProc::GetWndProc()
{
	return m_OriginalWndProc;
}

void Hook_WndProc::SetWndProc(WNDPROC lpPrevWndFunc)
{
	m_OriginalWndProc = lpPrevWndFunc;
}

void Hook_WndProc::Install(HWND hwnd)
{
	if (m_OriginalWndProc != nullptr) return;

	m_hWnd = hwnd;
	m_OriginalWndProc = (WNDPROC)SetWindowLongPtr(
		hwnd, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);

	s_Instance->m_Deps.System_Debug.Log("[WndProc] INFO: Hook installed.");
}

void Hook_WndProc::Uninstall()
{
	if (m_OriginalWndProc == nullptr) return;

	SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_OriginalWndProc);

	s_Instance->m_Deps.System_Debug.Log("[WndProc] INFO: Hook uninstalled.");

	m_OriginalWndProc = nullptr;
	m_hWnd = nullptr;
	s_Instance = nullptr;
}