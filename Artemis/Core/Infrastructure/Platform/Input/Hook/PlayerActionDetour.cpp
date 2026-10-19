module;

#include <windows.h>

module Platform.Input.Hook;
import :PlayerAction;

import Platform.Hook.System;
import Platform.Memory.Type;
import Platform.Simulation.Type;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Input::Type::PlayerActions;
    using Platform::Input::Type::ActionOverride;
    using Platform::Input::Type::ClientRequests;
    using Platform::Input::Type::WeaponRequest;
    using Platform::Simulation::Type::UnitClientState;
    using Platform::Simulation::Type::k_NoZoomLevel;

    using Platform::Input::Type::k_MaxActionPlayers;
    using Platform::Input::Type::k_PlayerActionSize;
    using Platform::Input::Type::k_UpdateActionMaskOffset;
    using Platform::Input::Type::k_UpdateActionsOffset;
    using Platform::Input::Type::k_ActionHasUnitOffset;
    using Platform::Input::Type::k_ActionUnitSlotOffset;
    using Platform::Input::Type::k_ActionButtonsOffset;
    using Platform::Input::Type::k_ActionFacingOffset;
    using Platform::Input::Type::k_ActionThrottleOffset;
    using Platform::Input::Type::k_ActionUseLaneOffset;
    using Platform::Input::Type::k_UsePressed;
    using Platform::Input::Type::k_UseHeld;
    using Platform::Input::Type::k_ActionInteractionOffset;
    using Platform::Input::Type::k_ActionInteractionSize;
    using Platform::Input::Type::k_ActionInventoryVersionOffset;
    using Platform::Input::Type::k_ActionWeaponSlotOffset;
    using Platform::Input::Type::k_ActionGrenadeTypeOffset;
    using Platform::Input::Type::k_ActionZoomLevelOffset;
    using Platform::Input::Type::k_ActionKeepZoomButton;
    using Platform::Input::Type::k_ControlGlobalsSlot;
    using Platform::Input::Type::k_ControlBlockStride;
    using Platform::Input::Type::k_ControlFacingOffset;

    using Platform::Simulation::Type::k_InteractionRecordCount;
    using Platform::Simulation::Type::k_InteractionRecordSize;

    static_assert(k_ActionInteractionSize == k_InteractionRecordCount * k_InteractionRecordSize,
        "The interaction of the action must hold the records the engine builds.");

    // Sanity limit of the thread local storage index. Anything above it is not a valid slot.
    constexpr std::uint32_t k_MaxTlsIndex{ 1000 };

    // Ticks a weapon request is sent while the unit does not hold the slot (half a second at 60 ticks per second).
    constexpr std::uint32_t k_WeaponRequestTicks{ 30 };

    // Use test (A9 M5d), in ticks of a cycle: pressed until the first end tick, held until the second, then released, so the
    // engine sees new presses, long holds and releases.
    constexpr std::uint64_t k_UseTestCycle{ 64 };
    constexpr std::uint64_t k_UseTestPressedEndTick{ 30 };
    constexpr std::uint64_t k_UseTestHeldEndTick{ 60 };

    // return: The use lane of the use test at that tick.
    auto GetUseTestLane(std::uint64_t tick) -> std::uint8_t
    {
        const std::uint64_t phase{ tick % k_UseTestCycle };
        if (phase < k_UseTestPressedEndTick) return k_UsePressed;
        if (phase < k_UseTestHeldEndTick) return k_UseHeld;
        return 0;
    }

    // Aims the action at the unit in that slot.
    auto WriteUnitSlot(std::uint8_t* action, std::uint8_t unitSlot) -> void
    {
        constexpr std::uint32_t hasUnit{ 1 };
        std::memcpy(action + k_ActionHasUnitOffset, &hasUnit, sizeof(hasUnit));
        action[k_ActionUnitSlotOffset] = unitSlot;
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
        // note: Taken every tick, so a press never fires late.
        std::array<ClientRequests, k_MaxActionPlayers> requests{};
        m_PlayerActionStore.TakeClientRequests(requests);

        // note: Without the thread context the unit slot is unknown, and a cleared action would leave the unit without control.
        if (!m_ClearPlayerAction || threadContext == 0)
        {
            m_WeaponRequests.fill(std::nullopt);
            return;
        }

        std::array<std::optional<ActionOverride>, k_MaxActionPlayers> overrides{};
        m_PlayerActionStore.CopyOverrides(overrides);

        std::uint32_t mask{ 0 };
        std::memcpy(&mask, update + k_UpdateActionMaskOffset, sizeof(mask));

        const std::uintptr_t controlGlobals{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ControlGlobalsSlot) };

        for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player)
        {
            const auto& actionOverride{ overrides[player] };
            if (!actionOverride)
            {
                m_WeaponRequests[player].reset();
                continue;
            }

            std::uint8_t* action{ update + k_UpdateActionsOffset + (player * k_PlayerActionSize) };

            // note: The engine only builds actions for local players. Every agent, local or bot, starts from the empty action of the
            // engine instead, so they all send the same fields (D119). A zeroed action would fail the validation.
            m_ClearPlayerAction(action);

            const std::optional<std::uint8_t> unitSlot{ m_BotService.GetUnitActionSlot(threadContext, player) };
            if (unitSlot)
            {
                WriteUnitSlot(action, *unitSlot);
            }

            const std::array<float, 2> facing{ actionOverride->Yaw, actionOverride->Pitch };
            const std::array<float, 2> throttle{ actionOverride->ThrottleForward, actionOverride->ThrottleLeft };

            // note: The zoom level of the action is marked absent unless the agent asks for one, so the unit keeps its zoom (D120).
            const std::uint64_t buttons{ actionOverride->Buttons | k_ActionKeepZoomButton };

            std::memcpy(action + k_ActionButtonsOffset, &buttons, sizeof(buttons));
            std::memcpy(action + k_ActionFacingOffset, facing.data(), sizeof(facing));
            std::memcpy(action + k_ActionThrottleOffset, throttle.data(), sizeof(throttle));

            if (actionOverride->IsUseTest)
            {
                action[k_ActionUseLaneOffset] = GetUseTestLane(m_Sequence);
            }

            this->WriteClientRequests(action, threadContext, player, requests[player]);

            // note: The engine builds the interaction once per frame and only for local players. Every agent gets it here from the
            // same engine functions, once per tick, so locals and bots are treated alike (A9 M5d, D118).
            m_BotService.WriteInteraction(threadContext, player,
                std::span<std::uint8_t>{ action + k_ActionInteractionOffset, k_ActionInteractionSize });

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

    auto PlayerActionDetour::WriteClientRequests(std::uint8_t* action, std::uintptr_t threadContext, std::size_t player,
        const ClientRequests& requests) -> void
    {
        std::optional<WeaponRequest>& weaponRequest{ m_WeaponRequests[player] };

        const std::optional<UnitClientState> state{ m_BotService.GetUnitClientState(threadContext, player) };
        if (!state)
        {
            weaponRequest.reset();
            return;
        }

        if (weaponRequest && (state->WeaponSlot == weaponRequest->Slot || weaponRequest->TicksLeft == 0
            || state->InventoryVersion != weaponRequest->InventoryVersion))
        {
            weaponRequest.reset();
        }

        if (requests.IsWeaponSwitch)
        {
            // note: A second press while a request is pending goes on from the requested slot, as a double tap does.
            UnitClientState from{ *state };
            if (weaponRequest) from.WeaponSlot = weaponRequest->Slot;

            const std::optional<std::int8_t> slot{ m_BotService.GetNextWeaponSlot(from) };
            if (slot)
            {
                weaponRequest = WeaponRequest{ .Slot = *slot, .InventoryVersion = state->InventoryVersion,
                    .TicksLeft = k_WeaponRequestTicks };
            }
        }

        std::optional<std::int8_t> zoomLevel{};

        if (weaponRequest)
        {
            --weaponRequest->TicksLeft;
            std::memcpy(action + k_ActionInventoryVersionOffset, &weaponRequest->InventoryVersion,
                sizeof(weaponRequest->InventoryVersion));
            action[k_ActionWeaponSlotOffset] = static_cast<std::uint8_t>(weaponRequest->Slot);

            // note: A local client leaves the zoom when it switches weapon (0x61a38). A zoom press during the switch is dropped.
            zoomLevel = k_NoZoomLevel;
        }
        else if (requests.IsZoom)
        {
            zoomLevel = m_BotService.GetNextZoomLevel(*state);
        }

        if (requests.IsGrenadeSwitch)
        {
            const std::optional<std::int8_t> type{ m_BotService.GetNextGrenadeType(*state) };
            if (type)
            {
                const auto value{ static_cast<std::uint16_t>(*type) };
                std::memcpy(action + k_ActionGrenadeTypeOffset, &value, sizeof(value));
            }
        }

        if (zoomLevel)
        {
            const auto value{ static_cast<std::uint16_t>(static_cast<std::int16_t>(*zoomLevel)) };
            std::memcpy(action + k_ActionZoomLevelOffset, &value, sizeof(value));

            std::uint64_t buttons{ 0 };
            std::memcpy(&buttons, action + k_ActionButtonsOffset, sizeof(buttons));
            buttons &= ~k_ActionKeepZoomButton;
            std::memcpy(action + k_ActionButtonsOffset, &buttons, sizeof(buttons));
        }
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

        m_ClearPlayerAction = reinterpret_cast<ClearPlayerActionFunction>(m_AOBService.FindPattern(Signature::ClearPlayerAction));
        if (!m_ClearPlayerAction)
        {
            m_LogsService.Message("[PlayerActionDetour] ERROR: Failed to find ClearPlayerAction. No action override will be"
                " applied.");
        }

        m_EngineTlsIndex = this->FindEngineTlsIndex();
        if (!m_EngineTlsIndex)
        {
            m_LogsService.Message("[PlayerActionDetour] WARNING: Failed to find the engine thread context. No action override"
                " will be applied and no bot can be added.");
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