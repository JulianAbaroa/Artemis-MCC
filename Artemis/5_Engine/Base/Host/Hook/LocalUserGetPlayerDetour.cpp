module Engine.Base.Host.Hook;
import :LocalUserGetPlayer;

import Engine.Base.Hook.System;
import std;

namespace
{
    using Engine::Base::Host::Type::k_LocalUserGetPlayerIndex;

    using Engine::Base::Hook::System::k_DrainTimeout;
}

namespace Engine::Base::Host::Hook
{
    auto __fastcall LocalUserGetPlayerDetour::HookedLocalUserGetPlayer(void* gameManager, std::uint64_t* xuid, wchar_t* name,
        std::uint32_t nameSize, std::int32_t player) -> bool
    {
        Engine::Base::Hook::System::InFlightScope scope(s_InFlight);

        auto* self{ s_Instance };
        auto& store{ self->m_LocalUserStore };

        if (player <= 0 || static_cast<std::size_t>(player) >= store.GetRequestedCount())
        {
            const bool isFound{ s_OriginalFunction(gameManager, xuid, name, nameSize, player) };

            if (player == 0 && isFound && xuid != nullptr) store.SetPrimaryXuid(*xuid);

            return isFound;
        }

        const auto user{ static_cast<std::size_t>(player) };

        if (xuid != nullptr) *xuid = store.GetXuid(user);

        // nameSize is in bytes (MCC halves it before copying).
        const std::size_t capacity{ nameSize / sizeof(wchar_t) };

        if (name != nullptr && capacity > 0)
        {
            const std::wstring_view userName{ store.GetName(user) };
            const std::size_t length{ (std::min)(userName.size(), capacity - 1) };

            std::copy_n(userName.data(), length, name);
            name[length] = L'\0';
        }

        if (store.MarkReported(user))
        {
            self->m_Log.Message("[LocalUserGetPlayerDetour] INFO: Local user {} reported to the engine.", user);
        }

        return true;
    }

    auto LocalUserGetPlayerDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress{ m_HostService.GetFunctionAddress(k_LocalUserGetPlayerIndex) };
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedLocalUserGetPlayer),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[LocalUserGetPlayerDetour]", m_Log))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto LocalUserGetPlayerDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());
        Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[LocalUserGetPlayerDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}