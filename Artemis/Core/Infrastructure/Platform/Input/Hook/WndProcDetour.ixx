module;

#include <windows.h>

export module Platform.Input.Hook:WndProc;

import Service.Logs.System;
import Platform.Input.System;
import Platform.Lifecycle.State;
import Platform.Lifecycle.System;
import std;

export namespace Platform::Input::Hook
{
	class WndProcDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using InputService = Platform::Input::System::InputService;
		using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
		using LifecycleService = Platform::Lifecycle::System::LifecycleService;

	public:
		WndProcDetour(LogsService& logsService, InputService& inputService, 
			LifecycleStore& lifecycleStore, LifecycleService& lifecycleService) :
			m_LogsService(logsService), m_InputService(inputService), 
			m_LifecycleStore(lifecycleStore), m_LifecycleService(lifecycleService) {}
		~WndProcDetour() = default;

		auto Install(HWND window) -> void;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		InputService& m_InputService;
		LifecycleStore& m_LifecycleStore;
		LifecycleService& m_LifecycleService;

		static WndProcDetour* s_Instance;

		static inline std::atomic<int> s_InFlight{ 0 };

		static auto CALLBACK HookedWndProc(HWND window, UINT message,
			WPARAM wParam, LPARAM lParam)->LRESULT;

		static inline std::atomic<WNDPROC> m_OriginalWndProc{ nullptr };
		static inline std::atomic<HWND> m_Window{ nullptr };
	};
}
