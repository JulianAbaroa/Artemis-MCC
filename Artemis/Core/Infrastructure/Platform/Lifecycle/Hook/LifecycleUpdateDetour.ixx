export module Platform.Lifecycle.Hook:LifecycleUpdate;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Lifecycle.Type;
import Platform.Lifecycle.State;
import std;

export namespace Platform::Lifecycle::Hook
{
    // Launches the match of the custom games lobby without its screen when Artemis requests it (D124): before the lifecycle update of
    // MCC runs, starts the countdown of the lobby with 0 seconds, so the PreGame state starts the match on that same update and MCC
    // goes on by its normal path (A15 sec. 2.14).
    // note: Hooks an MCC function that runs once per frame on the MCC thread and stays loaded for the whole process. Without a request
    // the hook only calls the original.
    class LifecycleUpdateDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AOBService = Platform::Memory::System::AOBService;

        using LifecycleUpdateFunction = Platform::Lifecycle::Type::LifecycleUpdateFunction;
        using StartCountdownFunction = Platform::Lifecycle::Type::StartCountdownFunction;
        using MatchConfigStore = Platform::Lifecycle::State::MatchConfigStore;

    public:
        LifecycleUpdateDetour(LogsService& logsService, AOBService& aobService, MatchConfigStore& matchConfigStore) :
            m_LogsService(logsService), m_AOBService(aobService), m_MatchConfigStore(matchConfigStore) {}
        ~LifecycleUpdateDetour() = default;

        LifecycleUpdateDetour(const LifecycleUpdateDetour&) = delete;
        auto operator=(const LifecycleUpdateDetour&) -> LifecycleUpdateDetour& = delete;

        // Finds the lifecycle update and the countdown function of MCC by signature and installs the hook. Does nothing if already
        // installed.
        // return: False if a function is not found or the hook fails.
        auto Install() -> bool;

        // Disables the hook, waits for the calls in flight, then removes it.
        // note: Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        MatchConfigStore& m_MatchConfigStore;

        static inline LifecycleUpdateDetour* s_Instance{ nullptr };
        static inline LifecycleUpdateFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        // MCC function behind the START of the lobby, found at install.
        StartCountdownFunction m_StartCountdown{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedLifecycleUpdate(void* manager) -> void;

        // Starts the countdown of the lobby if the lifecycle is in PreGame, or logs why it does not.
        // note: MCC thread, inside the hook, before the original update.
        auto Launch(const void* manager) -> void;
    };
}