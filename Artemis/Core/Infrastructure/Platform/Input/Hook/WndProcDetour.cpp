module;

#include <windows.h>

module Platform.Input.Hook;
import :WndProc;

import std;

import Platform.Input.Type;
import std;

namespace
{
	using WindowMessage = Platform::Input::Type::WindowMessage;

	struct InFlightScope
	{
		explicit InFlightScope(std::atomic<int>& counter) : m_Counter(counter) 
		{
			m_Counter.fetch_add(1, std::memory_order_acq_rel);
		}
		~InFlightScope()
		{
			m_Counter.fetch_sub(1, std::memory_order_acq_rel);
		}

		InFlightScope(const InFlightScope&) = delete;
		InFlightScope& operator=(const InFlightScope&) = delete;

		std::atomic<int>& m_Counter;
	};
}

namespace Platform::Input::Hook
{
	WndProcDetour* WndProcDetour::s_Instance = nullptr;

	auto CALLBACK WndProcDetour::HookedWndProc(HWND window, UINT message,
		WPARAM wParam, LPARAM lParam) -> LRESULT
	{
		const WNDPROC original = m_OriginalWndProc.load();
		if (!original) return DefWindowProcW(window, message, wParam, lParam);

		auto* self = s_Instance;
		if (!self) return CallWindowProcW(original, window, message, wParam, lParam);

		InFlightScope scope(s_InFlight);

		const bool windowDestroyed = (message == WM_CLOSE ||
			message == WM_DESTROY || message == WM_QUIT);

		if (windowDestroyed)
		{
			if (self->m_LifecycleStore.IsRunning())
			{
				self->m_LogsService.Message("[WndProcDetour] WARNING:"
					" MCC shutdown detected.");

				self->m_LifecycleService.SignalShutdown();
			}

			return CallWindowProcW(original, window, message, wParam, lParam);
		}

		const bool altF4Pressed = (message == WM_SYSKEYDOWN && wParam == VK_F4);
		if (altF4Pressed)
		{
			return CallWindowProcW(original, window, message, wParam, lParam);
		}

		WindowMessage input{};
		input.Window = window;
		input.Message = message;
		input.WParam = wParam;
		input.LParam = lParam;

		if (self->m_InputService.RaiseWindowMessage(input))
		{
			return static_cast<LRESULT>(input.Result);
		}

		return CallWindowProcW(original, window, message, wParam, lParam);
	}

	auto WndProcDetour::Install(HWND window) -> void
	{
		if (!window) return;

		if (m_OriginalWndProc.load() && m_Window.load() == window)
		{
			s_Instance = this;
			return;
		}

		if (m_OriginalWndProc.load()) this->Uninstall();

		s_Instance = this;

		SetLastError(0);
		const auto previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
			window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&HookedWndProc)));

		if (!previous && GetLastError() != 0)
		{ 
			m_LogsService.Message("[WndProcDetour] ERROR:"
				" Failed to hook the window procedure.");

			s_Instance = nullptr;
			return;
		}

		m_Window.store(window);
		m_OriginalWndProc.store(previous);

		m_LogsService.Message("[WndProcDetour] INFO: Hook installed.");
	}

	auto WndProcDetour::Uninstall() -> void
	{
		const WNDPROC original = m_OriginalWndProc.load();
		if (!original) return;

		s_Instance = nullptr;

		const HWND window = m_Window.load();

		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
		while (s_InFlight.load(std::memory_order_acquire) > 0 &&
			std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}

		const auto current = GetWindowLongPtrW(window, GWLP_WNDPROC);
		if (current == reinterpret_cast<LONG_PTR>(&HookedWndProc))
		{
			SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original));

			m_OriginalWndProc.store(nullptr);
			m_Window.store(nullptr);

			m_LogsService.Message("[WndProcDetour] INFO: Hook uninstalled.");
			return;
		}

		m_LogsService.Message("[WndProcDetour] WARNING: The window procedure was"
			" hooked by another module after Artemis. Left as passthrough.");
	}
}