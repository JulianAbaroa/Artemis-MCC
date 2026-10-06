export module Platform.Input.Hook:GetButtonState;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Input.Type;
import Platform.Input.State;
import std;

export namespace Platform::Input::Hook
{
    // Hooks the engine button polling to report AI requested actions as held. Other buttons pass through.
    class GetButtonStateDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using AOBService = Platform::Memory::System::AOBService;

        using GetButtonStateFunction = Platform::Input::Type::GetButtonStateFunction;

        using InputStore = Platform::Input::State::InputStore;

    public:
        GetButtonStateDetour(LogsService& logsService, AOBService& aobService,
            InputStore& inputStore) : m_LogsService(logsService),
            m_AOBService(aobService), m_InputStore(inputStore) {}
        ~GetButtonStateDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        auto Install() -> void;

        // Removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        InputStore& m_InputStore;

        static inline GetButtonStateDetour* s_Instance{ nullptr };
        static inline GetButtonStateFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedGetButtonState(short buttonID) -> char;
    };
}