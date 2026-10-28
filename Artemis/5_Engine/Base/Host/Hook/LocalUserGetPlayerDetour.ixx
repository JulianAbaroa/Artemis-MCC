export module Platform.Host.Hook:LocalUserGetPlayer;

import Common.Log.System;
import Platform.Host.Type;
import Platform.Host.State;
import Platform.Host.System;
import std;

export namespace Platform::Host::Hook
{
    // Hooks the MCC function that reports each local user to the engine, so the engine sees the Artemis users as split-screen players.
    // note: User 0 and slots above the requested count go to MCC. With a requested count of 1 the game is untouched.
    class LocalUserGetPlayerDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using LocalUserGetPlayerFunction = Platform::Host::Type::LocalUserGetPlayerFunction;

        using LocalUserStore = Platform::Host::State::LocalUserStore;

        using HostService = Platform::Host::System::HostService;

    public:
        LocalUserGetPlayerDetour(Log& log, HostService& hostService, LocalUserStore& localUserStore) :
            m_Log(log), m_HostService(hostService), m_LocalUserStore(localUserStore) {}
        ~LocalUserGetPlayerDetour() = default;

        // Finds the target in the game manager vtable and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        HostService& m_HostService;
        LocalUserStore& m_LocalUserStore;

        static inline LocalUserGetPlayerDetour* s_Instance{ nullptr };
        static inline LocalUserGetPlayerFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedLocalUserGetPlayer(void* gameManager, std::uint64_t* xuid, wchar_t* name,
            std::uint32_t nameSize, std::int32_t player) -> bool;
    };
}