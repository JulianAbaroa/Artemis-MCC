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
    // 2.14 and 2.16). Also publishes whether the lobby is open and sends the Leave a tool asks for (D129, D131).
    // After a match the lobby keeps the game variant of the match with its own map, and START refuses the pair as incompatible. The
    // engine start replaces both anyway (D123), so while Artemis presses START a second hook reports the variants as compatible.
    // note: Hooks an MCC function that runs once per frame on the MCC thread and stays loaded for the whole process. Without a request
    // the hook only looks for the lobby screen every few frames and calls the original.
    class LifecycleUpdateDetour
    {
    private:
        using steady_clock = std::chrono::steady_clock;

        using LogsService = Service::Logs::System::LogsService;

        using AOBService = Platform::Memory::System::AOBService;

        using LifecycleUpdateFunction = Platform::Lifecycle::Type::LifecycleUpdateFunction;
        using FindScreenFunction = Platform::Lifecycle::Type::FindScreenFunction;
        using LobbyStartFunction = Platform::Lifecycle::Type::LobbyStartFunction;
        using GetSessionFunction = Platform::Lifecycle::Type::GetSessionFunction;
        using IsCountdownActiveFunction = Platform::Lifecycle::Type::IsCountdownActiveFunction;
        using EngineUpdateStateFunction = Platform::Lifecycle::Type::EngineUpdateStateFunction;
        using VariantsCompatibleFunction = Platform::Lifecycle::Type::VariantsCompatibleFunction;
        using MatchConfigStore = Platform::Lifecycle::State::MatchConfigStore;
        using MatchCycleStore = Platform::Lifecycle::State::MatchCycleStore;

    public:
        LifecycleUpdateDetour(LogsService& logsService, AOBService& aobService, MatchConfigStore& matchConfigStore,
            MatchCycleStore& matchCycleStore) :
            m_LogsService(logsService), m_AOBService(aobService), m_MatchConfigStore(matchConfigStore),
            m_MatchCycleStore(matchCycleStore) {}
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
        MatchCycleStore& m_MatchCycleStore;

        static inline LifecycleUpdateDetour* s_Instance{ nullptr };
        static inline LifecycleUpdateFunction s_OriginalFunction{ nullptr };
        static inline VariantsCompatibleFunction s_OriginalVariantsCompatible{ nullptr };

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
        std::atomic<void*> m_VariantsAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        // True only while Launch calls the START of the lobby. MCC thread only.
        bool m_IsPressingStart{ false };

        // Frames since the lobby screen was last looked for, when it was last seen opening, and whether the pending launch already said
        // it waits. MCC thread only.
        std::uint32_t m_LobbyPollFrame{ 0 };
        steady_clock::time_point m_LobbyOpenSince{};
        bool m_IsLaunchWaiting{ false };

        static auto __fastcall HookedLifecycleUpdate(void* manager) -> void;

        // return: True while Artemis presses START, the answer of MCC otherwise.
        static auto __fastcall HookedVariantsCompatible(std::uint64_t first, std::uint64_t second, std::uint64_t third,
            std::uint64_t fourth) -> bool;

        // return: The custom games lobby screen if MCC is in PreGame and the screen is open, null otherwise.
        // note: The main menu is PreGame too: only the open screen tells that MCC is in the custom games lobby.
        auto FindLobby(const void* manager) const -> void*;

        // Looks for the lobby screen, publishes whether it is open and notes when it opened.
        // return: The screen, null if it is not open.
        auto UpdateLobby(const void* manager) -> void*;

        // Presses the START of the custom games lobby screen once it has been open for a moment and no countdown runs. Otherwise the
        // request waits for the lobby (MCC shows it again after every match) until it times out.
        // note: MCC thread, inside the hook, before the original update.
        auto Launch(const void* manager) -> void;

        // Sends the Leave state command to the live engine, as the pause menu does.
        // note: MCC thread, inside the hook, the thread that calls the engine interface.
        auto Leave() -> void;
    };
}