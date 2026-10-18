module;

#include <windows.h>

module Platform.Input.Hook;
import :PlayerAction;

import Platform.Hook.System;
import Platform.Memory.Type;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Input::Type::PlayerActions;
    using Platform::Input::Type::ActionOverride;

    using Platform::Input::Type::k_MaxActionPlayers;
    using Platform::Input::Type::k_PlayerActionSize;
    using Platform::Input::Type::k_UpdateActionMaskOffset;
    using Platform::Input::Type::k_UpdateActionsOffset;
    using Platform::Input::Type::k_ActionButtonsOffset;
    using Platform::Input::Type::k_ActionFacingOffset;
    using Platform::Input::Type::k_ActionThrottleOffset;
    using Platform::Input::Type::k_ControlGlobalsSlot;
    using Platform::Input::Type::k_ControlBlockStride;
    using Platform::Input::Type::k_ControlFacingOffset;

    // Sanity limit of the thread local storage index. Anything above it is not a valid slot.
    constexpr std::uint32_t k_MaxTlsIndex{ 1000 };

    // Fields of an action that the engine validation (0x46ac8) rejects when they are zero (A9 sec. 13.9). A player without an action
    // from the engine this tick (a bot) gets them as "nothing": no unit slot (+0x00 = 0 with +0x04 = 0xFF), no weapon or grenade
    // change (+0x46, +0x47 = 0xFF, the same value would be rejected), no grenade pick (+0x48 = 0xFFFF) and no zoom level (+0x4A = 0xFFFF).
    constexpr std::uintptr_t k_ActionUnitSlotOffset{ 0x04 };
    constexpr std::uintptr_t k_ActionWeaponOffset{ 0x46 };
    constexpr std::uintptr_t k_ActionSecondWeaponOffset{ 0x47 };
    constexpr std::uintptr_t k_ActionGrenadeOffset{ 0x48 };
    constexpr std::uintptr_t k_ActionZoomOffset{ 0x4A };

    // Marks an action as aimed at the unit in that slot (+0x00 = 1, +0x04 = slot), so the engine moves the unit with it.
    constexpr std::uintptr_t k_ActionHasUnitOffset{ 0x00 };

    // Writes an action that does nothing and passes the engine validation.
    // param unitSlot: Action slot of the unit of the player (unit +0x288), or nullopt without a unit.
    auto WriteNeutralAction(std::uint8_t* action, std::optional<std::uint8_t> unitSlot) -> void
    {
        std::memset(action, 0, k_PlayerActionSize);
        action[k_ActionUnitSlotOffset] = 0xFF;

        if (unitSlot)
        {
            constexpr std::uint32_t hasUnit{ 1 };
            std::memcpy(action + k_ActionHasUnitOffset, &hasUnit, sizeof(hasUnit));
            action[k_ActionUnitSlotOffset] = *unitSlot;
        }

        action[k_ActionWeaponOffset] = 0xFF;
        action[k_ActionSecondWeaponOffset] = 0xFF;
        std::memset(action + k_ActionGrenadeOffset, 0xFF, sizeof(std::uint16_t));
        std::memset(action + k_ActionZoomOffset, 0xFF, sizeof(std::uint16_t));
    }
}

namespace Platform::Input::Hook
{
    auto __fastcall PlayerActionDetour::HookedApplyPlayerActions(void* update) -> void
    {
        Platform::Hook::System::InFlightScope scope(s_InFlight);

        if (update != nullptr)
        {
            auto* bytes{ static_cast<std::uint8_t*>(update) };
            const std::uintptr_t threadContext{ s_Instance->GetThreadContext() };

            s_Instance->m_BotService.Update(threadContext);
            s_Instance->ApplyOverrides(bytes, threadContext);
            s_Instance->PublishActions(bytes);
        }

        s_OriginalFunction(update);
    }

    auto PlayerActionDetour::ApplyOverrides(std::uint8_t* update, std::uintptr_t threadContext) -> void
    {
        std::array<std::optional<ActionOverride>, k_MaxActionPlayers> overrides{};
        m_PlayerActionStore.CopyOverrides(overrides);

        std::uint32_t mask{ 0 };
        std::memcpy(&mask, update + k_UpdateActionMaskOffset, sizeof(mask));

        const std::uintptr_t controlGlobals{ threadContext != 0
            ? *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ControlGlobalsSlot) : 0 };

        for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player)
        {
            const auto& actionOverride{ overrides[player] };
            if (!actionOverride) continue;

            std::uint8_t* action{ update + k_UpdateActionsOffset + (player * k_PlayerActionSize) };

            // note: The engine only builds actions for local players. A bot has none, and a zeroed action fails the validation.
            if ((mask & (1u << player)) == 0)
            {
                WriteNeutralAction(action, m_BotService.GetUnitActionSlot(threadContext, player));
            }

            const std::array<float, 2> facing{ actionOverride->Yaw, actionOverride->Pitch };
            const std::array<float, 2> throttle{ actionOverride->ThrottleForward, actionOverride->ThrottleLeft };

            std::memcpy(action + k_ActionButtonsOffset, &actionOverride->Buttons, sizeof(actionOverride->Buttons));
            std::memcpy(action + k_ActionFacingOffset, facing.data(), sizeof(facing));
            std::memcpy(action + k_ActionThrottleOffset, throttle.data(), sizeof(throttle));

            mask |= 1u << player;

            if (controlGlobals != 0 && actionOverride->LocalIndex)
            {
                auto* block{ reinterpret_cast<std::uint8_t*>(controlGlobals + (*actionOverride->LocalIndex * k_ControlBlockStride)) };
                std::memcpy(block + k_ControlFacingOffset, facing.data(), sizeof(facing));
            }

            if (actionOverride->YawPerTick != 0.0f)
            {
                m_PlayerActionStore.AdvanceOverrideYaw(player, actionOverride->YawPerTick);
            }
        }

        std::memcpy(update + k_UpdateActionMaskOffset, &mask, sizeof(mask));
    }

    auto PlayerActionDetour::GetThreadContext() const -> std::uintptr_t
    {
        if (!m_EngineTlsIndex) return 0;

        const std::uint32_t tlsIndex{ *m_EngineTlsIndex };
        if (tlsIndex > k_MaxTlsIndex) return 0;

        // note: gs:[0x58] is the thread local storage array of the current thread.
        const auto tlsArray{ static_cast<std::uintptr_t>(__readgsqword(0x58)) };

        return *reinterpret_cast<const std::uintptr_t*>(tlsArray + (tlsIndex * sizeof(std::uintptr_t)));
    }

    auto PlayerActionDetour::FindEngineTlsIndex() -> const std::uint32_t*
    {
        const std::uintptr_t match{ m_AOBService.FindPattern(Signature::TelemetryIdModifier) };
        if (!match) return nullptr;

        // note: The match is a "mov ecx, [rip+rel32]". The rel32 is at +2 and the instruction is 6 bytes long.
        const std::int32_t relativeOffset{ *reinterpret_cast<const std::int32_t*>(match + 2) };
        return reinterpret_cast<const std::uint32_t*>((match + 6) + relativeOffset);
    }

    auto PlayerActionDetour::PublishActions(const std::uint8_t* update) -> void
    {
        PlayerActions actions{};
        actions.Sequence = ++m_Sequence;
        std::memcpy(&actions.Mask, update + k_UpdateActionMaskOffset, sizeof(actions.Mask));

        for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player)
        {
            std::memcpy(actions.Actions[player].Data.data(),
                update + k_UpdateActionsOffset + (player * k_PlayerActionSize), k_PlayerActionSize);
        }

        m_PlayerActionStore.Publish(actions);
    }

    auto PlayerActionDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;
        m_Sequence = 0;

        m_EngineTlsIndex = this->FindEngineTlsIndex();
        if (!m_EngineTlsIndex)
        {
            m_LogsService.Message("[PlayerActionDetour] WARNING: Failed to find the engine thread context. Views will not follow"
                " the overrides and no bot can be added.");
        }

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