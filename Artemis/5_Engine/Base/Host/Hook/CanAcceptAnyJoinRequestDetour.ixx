export module Engine.Base.Host.Hook:CanAcceptAnyJoinRequest;

import Common.Log.System;
import Engine.Base.Memory.System;
import Engine.Base.Host.Type;
import Engine.Base.Host.State;
import std;

export namespace Engine::Base::Host::Hook
{
    // Hooks the engine check that decides if the network session accepts join requests, so the Artemis users can join the match.
    // note: Same change as AlphaRing (it patches the function to return 0). Only applies when more than one local user is requested.
    class CanAcceptAnyJoinRequestDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using CanAcceptAnyJoinRequestFunction = Engine::Base::Host::Type::CanAcceptAnyJoinRequestFunction;

        using LocalUserStore = Engine::Base::Host::State::LocalUserStore;

        using AOBService = Engine::Base::Memory::System::AOBService;

    public:
        CanAcceptAnyJoinRequestDetour(Log& log, AOBService& aobService, LocalUserStore& localUserStore) :
            m_Log(log), m_AOBService(aobService), m_LocalUserStore(localUserStore) {}
        ~CanAcceptAnyJoinRequestDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        LocalUserStore& m_LocalUserStore;

        static inline CanAcceptAnyJoinRequestDetour* s_Instance{ nullptr };
        static inline CanAcceptAnyJoinRequestFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedCanAcceptAnyJoinRequest(void* session) -> std::int32_t;
    };
}