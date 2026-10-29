export module Engine.Base.Host.Hook:LocalUserGetGamepadMapping;

import Common.Log.System;
import Engine.Base.Host.Type;
import Engine.Base.Host.State;
import Engine.Base.Host.System;
import std;

export namespace Engine::Base::Host::Hook
{
    // Hooks the MCC function that returns the gamepad mapping of a user, so an Artemis user gets a valid mapping.
    // note: MCC does not know the Artemis XUIDs. Their mapping is a copy of the mapping of user 0.
    class LocalUserGetGamepadMappingDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using LocalUserGetGamepadMappingFunction = Engine::Base::Host::Type::LocalUserGetGamepadMappingFunction;
        using GamepadMapping = Engine::Base::Host::Type::GamepadMapping;

        using LocalUserStore = Engine::Base::Host::State::LocalUserStore;

        using HostService = Engine::Base::Host::System::HostService;

    public:
        LocalUserGetGamepadMappingDetour(Log& log, HostService& hostService, LocalUserStore& localUserStore) :
            m_Log(log), m_HostService(hostService), m_LocalUserStore(localUserStore) {}
        ~LocalUserGetGamepadMappingDetour() = default;

        // Finds the target in the game manager vtable and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        HostService& m_HostService;
        LocalUserStore& m_LocalUserStore;

        static inline LocalUserGetGamepadMappingDetour* s_Instance{ nullptr };
        static inline LocalUserGetGamepadMappingFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedLocalUserGetGamepadMapping(void* gameManager, std::uint64_t xuid) -> GamepadMapping*;
    };
}