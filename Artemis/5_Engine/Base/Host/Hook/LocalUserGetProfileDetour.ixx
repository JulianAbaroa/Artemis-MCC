export module Engine.Base.Host.Hook:LocalUserGetProfile;

import Common.Log.System;
import Engine.Base.Host.Type;
import Engine.Base.Host.State;
import Engine.Base.Host.System;
import std;

export namespace Engine::Base::Host::Hook
{
    // Hooks the MCC function that returns the profile of a user, so an Artemis user gets a valid profile.
    // note: MCC does not know the Artemis XUIDs. Their profile is a copy of the profile of user 0 for now. The AI will get a fixed
    // profile of its own: sensitivity, dead zones and bindings are part of the meaning of its actions (Q67).
    class LocalUserGetProfileDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using LocalUserGetProfileFunction = Engine::Base::Host::Type::LocalUserGetProfileFunction;
        using PlayerProfile = Engine::Base::Host::Type::PlayerProfile;

        using LocalUserStore = Engine::Base::Host::State::LocalUserStore;

        using HostService = Engine::Base::Host::System::HostService;

    public:
        LocalUserGetProfileDetour(Log& log, HostService& hostService, LocalUserStore& localUserStore) :
            m_Log(log), m_HostService(hostService), m_LocalUserStore(localUserStore) {}
        ~LocalUserGetProfileDetour() = default;

        // Finds the target in the game manager vtable and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        HostService& m_HostService;
        LocalUserStore& m_LocalUserStore;

        static inline LocalUserGetProfileDetour* s_Instance{ nullptr };
        static inline LocalUserGetProfileFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedLocalUserGetProfile(void* gameManager, std::uint64_t xuid) -> PlayerProfile*;
    };
}