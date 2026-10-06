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
    // Subclasses the game window procedure to forward window messages to the input handlers.
    // note: Signals shutdown when the window is closed or destroyed.
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

        // Replaces the window procedure of the given window. Does nothing if already installed on it.
        // note: If installed on another window, that one is restored first.
        auto Install(HWND window) -> void;

        // Restores the original window procedure.
        // note: If another module hooked the window after Artemis, the hook is left as a passthrough.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        InputService& m_InputService;
        LifecycleStore& m_LifecycleStore;
        LifecycleService& m_LifecycleService;

        static inline WndProcDetour* s_Instance{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        static inline std::atomic<WNDPROC> s_OriginalWndProc{ nullptr };
        static inline std::atomic<HWND> s_Window{ nullptr };

        static auto CALLBACK HookedWndProc(HWND window, UINT message,
            WPARAM wParam, LPARAM lParam) -> LRESULT;
    };
}