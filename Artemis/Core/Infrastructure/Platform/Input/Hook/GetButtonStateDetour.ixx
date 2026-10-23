export module Platform.Input.Hook:GetButtonState;

import Common.Log.System;
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
        using Log = Common::Log::System::Log;
        using AOBService = Platform::Memory::System::AOBService;

        using GetButtonStateFunction = Platform::Input::Type::GetButtonStateFunction;

        using InputStore = Platform::Input::State::InputStore;

    public:
        GetButtonStateDetour(Log& log, AOBService& aobService,
            InputStore& inputStore) : m_Log(log),
            m_AOBService(aobService), m_InputStore(inputStore) {}
        ~GetButtonStateDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        auto Install() -> void;

        // Removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        InputStore& m_InputStore;

        static inline GetButtonStateDetour* s_Instance{ nullptr };
        static inline std::atomic<int> s_InFlight{ 0 };
        static inline GetButtonStateFunction s_OriginalFunction{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedGetButtonState(short buttonID) -> char;
    };
}