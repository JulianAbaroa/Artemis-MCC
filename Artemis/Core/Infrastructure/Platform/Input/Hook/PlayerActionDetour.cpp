module Platform.Input.Hook;
import :PlayerAction;

import Platform.Hook.System;
import Platform.Memory.Type;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Input::Type::PlayerActions;

    using Platform::Input::Type::k_MaxActionPlayers;
    using Platform::Input::Type::k_PlayerActionSize;
    using Platform::Input::Type::k_UpdateActionMaskOffset;
    using Platform::Input::Type::k_UpdateActionsOffset;
}

namespace Platform::Input::Hook
{
    auto __fastcall PlayerActionDetour::HookedApplyPlayerActions(void* update) -> void
    {
        Platform::Hook::System::InFlightScope scope(s_InFlight);

        if (update != nullptr)
        {
            const auto* bytes{ static_cast<const std::uint8_t*>(update) };

            PlayerActions actions{};
            actions.Sequence = ++s_Instance->m_Sequence;
            std::memcpy(&actions.Mask, bytes + k_UpdateActionMaskOffset, sizeof(actions.Mask));

            for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player)
            {
                std::memcpy(actions.Actions[player].Data.data(),
                    bytes + k_UpdateActionsOffset + (player * k_PlayerActionSize), k_PlayerActionSize);
            }

            s_Instance->m_PlayerActionStore.Publish(actions);
        }

        s_OriginalFunction(update);
    }

    auto PlayerActionDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;
        m_Sequence = 0;

        void* functionAddress{ reinterpret_cast<void*>(m_AOBService.FindPattern(Signature::ApplyPlayerActions)) };
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedApplyPlayerActions),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[PlayerActionDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto PlayerActionDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        Platform::Hook::System::WaitForDrain(s_InFlight, std::chrono::milliseconds(500));
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[PlayerActionDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}