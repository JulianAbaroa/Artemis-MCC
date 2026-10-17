module Platform.Host.Hook;
import :LocalUserGetGamepadMapping;

import Platform.Hook.System;
import std;

namespace
{
    using Platform::Host::Type::k_LocalUserGetGamepadMappingIndex;
}

namespace Platform::Host::Hook
{
    auto __fastcall LocalUserGetGamepadMappingDetour::HookedLocalUserGetGamepadMapping(void* gameManager,
        std::uint64_t xuid) -> GamepadMapping*
    {
        Platform::Hook::System::InFlightScope scope(s_InFlight);

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

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedLocalUserGetGamepadMapping),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[LocalUserGetGamepadMappingDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto LocalUserGetGamepadMappingDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        Platform::Hook::System::WaitForDrain(s_InFlight, std::chrono::milliseconds(500));
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[LocalUserGetGamepadMappingDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}