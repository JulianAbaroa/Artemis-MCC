export module Platform.Input.Hook:InputUpdate;

import Common.Log.System;
import Platform.Host.System;
import Platform.Input.Type;
import Platform.Input.State;
import std;

export namespace Platform::Input::Hook
{
    // Hooks the MCC function that gives the engine the input of each local player (keyboard, mouse and gamepad), once per frame.
    // note: Players with LocalInputSource::Virtual get the state of the VirtualInputStore. The rest go to MCC.
    // note: Called by Input_UpdatePollingAndState of haloreach.dll for players 0 to 3 on the game thread.
    class InputUpdateDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using HostService = Platform::Host::System::HostService;

        using InputUpdateFunction = Platform::Input::Type::InputUpdateFunction;
        using InputState = Platform::Input::Type::InputState;

        using VirtualInputStore = Platform::Input::State::VirtualInputStore;

    public:
        InputUpdateDetour(Log& log, HostService& hostService, VirtualInputStore& virtualInputStore) :
            m_Log(log), m_HostService(hostService), m_VirtualInputStore(virtualInputStore) {}
        ~InputUpdateDetour() = default;

        // Finds the target in the game manager vtable and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        HostService& m_HostService;
        VirtualInputStore& m_VirtualInputStore;

        static inline InputUpdateDetour* s_Instance{ nullptr };
        static inline InputUpdateFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedInputUpdate(void* gameManager, std::int32_t player, InputState* state) -> bool;
    };
}