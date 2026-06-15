#include "pch.h"

#include "Hook_WndProc.h"

#include "Core/States/Other/Memory/State_MemoryScanner.h"
#include "Core/States/Other/Lifecycle/State_Lifecycle.h"
#include "Core/States/Other/Settings/State_Settings.h"
#include "Core/States/Other/Render/State_FlyCamera.h"
#include "Core/States/Other/Render/State_Selection.h"
#include "Core/States/Other/Render/State_OverlayMode.h"

#include "Core/Systems/Other/Memory/Scanner/System_MemoryScanner.h"
#include "Core/Systems/Other/Lifecycle/System_Lifecycle.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

#include "Core/UI/Launcher/UI_Launcher.h"
#include "Core/UI/Launcher/UI_Tab.h"
#include "Core/UI/Overlay/UI_Overlay.h"

#include "External/imgui/imgui.h"

#include <chrono>

using namespace std::chrono_literals;

LRESULT __stdcall Hook_WndProc::HookedWndProc(
	const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	if (s_Instance == nullptr)
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	bool windowDestroyed = (uMsg == WM_CLOSE || 
		uMsg == WM_DESTROY || uMsg == WM_QUIT);

	if (windowDestroyed)
	{
		if (s_Instance->m_Deps.State_Lifecycle.IsRunning())
		{
			s_Instance->m_Deps.System_Logs.Log("[WndProc] WARNING:"
				" MCC shutdown detected.");

			s_Instance->m_Deps.System_Lifecycle.SignalShutdown();
		}
		
		return CallWindowProc(m_OriginalWndProc, 
			hWnd, uMsg, wParam, lParam);
	}

	bool altF4Pressed = (uMsg == WM_SYSKEYDOWN && wParam == VK_F4);
	if (altF4Pressed)
	{
		return CallWindowProc(m_OriginalWndProc, 
			hWnd, uMsg, wParam, lParam);
	}

	bool isKeyDown = (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN);
	if (isKeyDown) if (HandleHotKeys(wParam)) return 0;


	bool anyVisible = UI_Tab::s_VisibleCount > 0;

	bool launcherVisible = 
		s_Instance->m_Deps.UI_Launcher.IsVisible();

	if (anyVisible || launcherVisible)
	{
		if (ImGui_ImplWin32_WndProcHandler(
			hWnd, uMsg, wParam, lParam)) return 1;

		bool wantCaptureKeyboard = 
			ImGui::GetIO().WantCaptureKeyboard;

		bool isKeyboardMessage = 
			(uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST);

		if (wantCaptureKeyboard && isKeyboardMessage) return 0;

		if (s_Instance->m_Deps.State_Settings.ShouldFreezeMouse())
		{
			switch (uMsg)
			{
			case WM_LBUTTONDOWN: case WM_LBUTTONUP: 
			case WM_LBUTTONDBLCLK: case WM_RBUTTONDOWN: 
			case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN: case WM_MBUTTONUP: 
			case WM_MBUTTONDBLCLK: case WM_MOUSEWHEEL:  
			case WM_MOUSEHWHEEL:
				return 0;
			}
		}
		else
		{
			bool wantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
			if (wantCaptureMouse)
			{
				switch (uMsg)
				{
				case WM_LBUTTONDOWN: case WM_LBUTTONUP: 
				case WM_LBUTTONDBLCLK: case WM_RBUTTONDOWN: 
				case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
				case WM_MBUTTONDOWN: case WM_MBUTTONUP: 
				case WM_MBUTTONDBLCLK: case WM_MOUSEWHEEL:  
				case WM_MOUSEHWHEEL:
					return 0;
				}
			}
		}
	}

	if (s_Instance->m_Deps.UI_Overlay.IsVisible() &&
		s_Instance->m_Deps.State_FlyCamera.IsActive() &&
		!anyVisible && !launcherVisible)
	{
		if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
		{
			if (wParam == VK_RIGHT)
			{
				s_Instance->m_Deps.State_OverlayMode.Next();
				return 0;
			}
			if (wParam == VK_LEFT)
			{
				s_Instance->m_Deps.State_OverlayMode.Prev();
				return 0;
			}
			if (wParam == VK_DOWN)
			{
				s_Instance->m_Deps.State_OverlayMode.NextPage();
				return 0;
			}
			if (wParam == VK_UP)
			{
				s_Instance->m_Deps.State_OverlayMode.PrevPage();
				return 0;
			}
		}
	}

	if (s_Instance->m_Deps.State_FlyCamera.IsActive() &&
		!anyVisible && !launcherVisible)
	{
		if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
		{
			s_Instance->m_Deps.State_FlyCamera.SetKey(wParam, true);
			return 0;
		}
		if (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP)
		{
			s_Instance->m_Deps.State_FlyCamera.SetKey(wParam, false);
			return 0;
		}
		if (uMsg == WM_LBUTTONDOWN)
		{
			s_Instance->m_Deps.State_Selection.RequestPick();
			return 0;
		}
	}

	return CallWindowProc(m_OriginalWndProc, hWnd, uMsg, wParam, lParam);
}

bool Hook_WndProc::HandleHotKeys(WPARAM wParam)
{
	if (wParam == VK_F2)
	{
		s_Instance->m_Deps.UI_Launcher.ToggleVisible();
		return true;
	}

	if (wParam == VK_F3)
	{
		s_Instance->m_Deps.UI_Overlay.ToggleVisible();
		return true;
	}

	if (wParam == VK_F4)
	{
		auto& fc = s_Instance->m_Deps.State_FlyCamera;
		bool now = !fc.IsActive();
		fc.SetActive(now);
		if (!now) fc.ResetKeys();
		return true;
	}

	if (wParam == VK_F5)
	{
		auto& fc = s_Instance->m_Deps.State_FlyCamera;
		fc.SetFollowEnabled(!fc.FollowEnabled());
		return true;
	}

	bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	if (shiftPressed)
	{
		if (wParam == '1')
		{
			s_Instance->m_Deps.UI_Launcher.ToggleAllTabs();
			return true;
		}

		if (wParam == '2')
		{
			s_Instance->m_Deps.UI_Launcher.ResetTabs();
			return true;
		}

		if (wParam == '3')
		{
			UI_Tab::s_Locked = !UI_Tab::s_Locked;
			return true;
		}
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

	s_Instance->m_Deps.System_Logs.Log("[WndProc] INFO: Hook installed.");
}

void Hook_WndProc::Uninstall()
{
	if (m_OriginalWndProc == nullptr) return;

	SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_OriginalWndProc);

	s_Instance->m_Deps.System_Logs.Log("[WndProc] INFO: Hook uninstalled.");

	m_OriginalWndProc = nullptr;
	m_hWnd = nullptr;
	s_Instance = nullptr;
}