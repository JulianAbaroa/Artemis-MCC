export module Platform.Lifecycle.Hook:LifecycleUpdate;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Lifecycle.Type;
import Platform.Lifecycle.State;
import std;

export namespace Platform::Lifecycle::Hook
{
    // Launches the match of the custom games lobby when Artemis requests it (D124): before the lifecycle update of MCC runs, calls
    // the START of the lobby screen, as pressing it does. MCC then counts down and launches the match by its normal path (A15 sec.
    // 2.14 and 2.16).
    // note: Hooks an MCC function that runs once per frame on the MCC thread and stays loaded for the whole process. Without a request
    // the hook only calls the original.
    class LifecycleUpdateDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AOBService = Platform::Memory::System::AOBService;

        using LifecycleUpdateFunction = Platform::Lifecycle::Type::LifecycleUpdateFunction;
        using FindScreenFunction = Platform::Lifecycle::Type::FindScreenFunction;
        using LobbyStartFunction = Platform::Lifecycle::Type::LobbyStartFunction;
        using GetSessionFunction = Platform::Lifecycle::Type::GetSessionFunction;
        using IsCountdownActiveFunction = Platform::Lifecycle::Type::IsCountdownActiveFunction;
        using MatchConfigStore = Platform::Lifecycle::State::MatchConfigStore;

    public:
        LifecycleUpdateDetour(LogsService& logsService, AOBService& aobService, MatchConfigStore& matchConfigStore) :
            m_LogsService(logsService), m_AOBService(aobService), m_MatchConfigStore(matchConfigStore) {}
        ~LifecycleUpdateDetour() = default;

        LifecycleUpdateDetour(const LifecycleUpdateDetour&) = delete;
        auto operator=(const LifecycleUpdateDetour&) -> LifecycleUpdateDetour& = delete;

        // Finds the lifecycle update, the lobby screen lookup and the countdown check of MCC by signature and installs the hook. Does nothing if already
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

        // UI manager global of MCC, name of the custom games lobby screen and the function that finds a screen, read at install.
        void** m_UiManagerGlobal{ nullptr };
        const char* m_LobbyScreenName{ nullptr };
        FindScreenFunction m_FindScreen{ nullptr };

        // Session getter and countdown check of MCC, read at install. The START of the lobby cancels nothing, so a running countdown
        // is checked first.
        GetSessionFunction m_GetSession{ nullptr };
        IsCountdownActiveFunction m_IsCountdownActive{ nullptr };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedLifecycleUpdate(void* manager) -> void;

        // Presses the START of the custom games lobby screen if it is open, the lifecycle is in PreGame and no countdown runs, or logs
        // why it does not.
        // note: MCC thread, inside the hook, before the original update.
        auto Launch(const void* manager) -> void;
    };
}