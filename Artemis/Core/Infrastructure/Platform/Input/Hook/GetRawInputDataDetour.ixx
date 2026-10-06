module;

#include <windows.h>

export module Platform.Input.Hook:GetRawInputData;

import Service.Logs.System;
import Service.Settings.State;
import Platform.Input.Type;
import Platform.Input.State;
import Platform.Input.System;
import std;

export namespace Platform::Input::Hook
{
    // Hooks GetRawInputData to filter mouse input.
    // note: Serves AI injected movement, freezes the mouse while the menu is open, and lets handlers swallow movement.
    class GetRawInputDataDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using SettingsStore = Service::Settings::State::SettingsStore;

        using GetRawInputDataFunction = Platform::Input::Type::GetRawInputDataFunction;

        using MouseDeltaStore = Platform::Input::State::MouseDeltaStore;

        using InputService = Platform::Input::System::InputService;

    public:
        GetRawInputDataDetour(LogsService& logsService, SettingsStore& settingsStore,
            InputService& inputService, MouseDeltaStore& mouseDeltaStore) :
            m_LogsService(logsService), m_SettingsStore(settingsStore),
            m_InputService(inputService), m_MouseDeltaStore(mouseDeltaStore) {}
        ~GetRawInputDataDetour() = default;

        // Hooks GetRawInputData from user32. Does nothing if already installed.
        auto Install() -> void;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        SettingsStore& m_SettingsStore;
        InputService& m_InputService;
        MouseDeltaStore& m_MouseDeltaStore;

        static inline GetRawInputDataDetour* s_Instance{ nullptr };
        static inline GetRawInputDataFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto WINAPI HookedGetRawInputData(HRAWINPUT hRawInput,
            UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) -> UINT;
    };
}