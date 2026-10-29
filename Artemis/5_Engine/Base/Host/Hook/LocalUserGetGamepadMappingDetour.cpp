module Engine.Base.Host.Hook;
import :LocalUserGetGamepadMapping;

import Engine.Base.Hook.System;
import std;

namespace
{
    using Engine::Base::Host::Type::k_LocalUserGetGamepadMappingIndex;

    using Engine::Base::Hook::System::k_DrainTimeout;
}

namespace Engine::Base::Host::Hook
{
    auto __fastcall LocalUserGetGamepadMappingDetour::HookedLocalUserGetGamepadMapping(void* gameManager,
        std::uint64_t xuid) -> GamepadMapping*
    {
        Engine::Base::Hook::System::InFlightScope scope(s_InFlight);

        auto& store{ s_Instance->m_LocalUserStore };

        const std::optional<std::size_t> user{ store.FindUser(xuid) };
        if (!user) return s_OriginalFunction(gameManager, xuid);

        GamepadMapping& mapping{ store.GetGamepadMapping(*user) };

        // The original only knows real users, so it is asked for the mapping of user 0.
        const std::uint64_t primaryXuid{ store.GetPrimaryXuid() };
        const GamepadMapping* primary{ primaryXuid != 0 ? s_OriginalFunction(gameManager, primaryXuid) : nullptr };

        if (primary != nullptr) mapping = *primary;

        return &mapping;
    }

    auto LocalUserGetGamepadMappingDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress{ m_HostService.GetFunctionAddress(k_LocalUserGetGamepadMappingIndex) };
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedLocalUserGetGamepadMapping),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[LocalUserGetGamepadMappingDetour]", m_Log))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto LocalUserGetGamepadMappingDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());
        Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[LocalUserGetGamepadMappingDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}