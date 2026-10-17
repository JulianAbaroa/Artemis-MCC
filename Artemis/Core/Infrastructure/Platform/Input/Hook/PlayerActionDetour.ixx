export module Platform.Input.Hook:PlayerAction;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Input.Type;
import Platform.Input.State;
import std;

export namespace Platform::Input::Hook
{
    // Hooks the engine function that applies the player actions of each tick, to read them (A9 M4a).
    // note: Does not change the actions yet. Writing the action of the AI here is A9 M4b (D109).
    // note: Called by ExecuteGameFrame once per tick on the game thread.
    class PlayerActionDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AOBService = Platform::Memory::System::AOBService;

        using ApplyPlayerActionsFunction = Platform::Input::Type::ApplyPlayerActionsFunction;

        using PlayerActionStore = Platform::Input::State::PlayerActionStore;

    public:
        PlayerActionDetour(LogsService& logsService, AOBService& aobService, PlayerActionStore& playerActionStore) :
            m_LogsService(logsService), m_AOBService(aobService), m_PlayerActionStore(playerActionStore) {}
        ~PlayerActionDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        PlayerActionStore& m_PlayerActionStore;

        static inline PlayerActionDetour* s_Instance{ nullptr };
        static inline ApplyPlayerActionsFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        // Ticks seen since the hook was installed. Game thread only.
        std::uint64_t m_Sequence{ 0 };

        static auto __fastcall HookedApplyPlayerActions(void* update) -> void;
    };
}