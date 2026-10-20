module;

#include <windows.h>

module Platform.Input.Hook;
import :PlayerAction;

import Platform.Lifecycle.Type;
import Platform.Hook.System;
import Platform.Memory.Type;
import Platform.Simulation.Type;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Hook::System::k_DrainTimeout;

    using Platform::Hook::System::ResolveRelative;

    using Platform::Input::Type::PlayerActions;
    using Platform::Input::Type::ActionOverride;
    using Platform::Input::Type::ClientRequests;
    using Platform::Input::Type::WeaponRequest;
    using Platform::Input::Type::ShadowField;
    using Platform::Simulation::Type::UnitClientState;
    using Platform::Simulation::Type::k_NoZoomLevel;

    using Platform::Lifecycle::Type::k_TicksPerSecond;

    using Platform::Input::Type::k_MaxActionPlayers;
    using Platform::Input::Type::k_PlayerActionSize;
    using Platform::Input::Type::k_UpdateActionMaskOffset;
    using Platform::Input::Type::k_UpdateActionsOffset;
    using Platform::Input::Type::k_ActionHasUnitOffset;
    using Platform::Input::Type::k_ActionUnitSlotOffset;
    using Platform::Input::Type::k_ActionButtonsOffset;
    using Platform::Input::Type::k_ActionFacingOffset;
    using Platform::Input::Type::k_ActionThrottleOffset;
    using Platform::Input::Type::k_ActionLanesOffset;
    using Platform::Input::Type::k_HoldLanes;
    using Platform::Input::Type::k_UseLane;
    using Platform::Input::Type::k_PlayerControlTagOffset;
    using Platform::Input::Type::k_HoldThresholdOffset;
    using Platform::Input::Type::k_TagLoadTableRelOffset;
    using Platform::Input::Type::k_TagLoadTableNextOffset;
    using Platform::Input::Type::k_TagLoadCallRelOffset;
    using Platform::Input::Type::k_TagLoadCallNextOffset;
    using Platform::Input::Type::k_ActionInteractionOffset;
    using Platform::Input::Type::k_ActionInteractionSize;
    using Platform::Input::Type::k_ActionInventoryVersionOffset;
    using Platform::Input::Type::k_ActionWeaponSlotOffset;
    using Platform::Input::Type::k_ActionGrenadeTypeOffset;
    using Platform::Input::Type::k_ActionZoomLevelOffset;
    using Platform::Input::Type::k_ActionKeepZoomButton;
    using Platform::Input::Type::k_ShadowFieldNames;
    using Platform::Input::Type::k_ControlGlobalsSlot;
    using Platform::Input::Type::k_ControlBlockStride;
    using Platform::Input::Type::k_ControlFacingOffset;

    using Platform::Simulation::Type::k_InteractionRecordCount;
    using Platform::Simulation::Type::k_InteractionRecordSize;
    using Platform::Simulation::Type::k_InteractionRecordObject;

    static_assert(k_ActionInteractionSize == k_InteractionRecordCount * k_InteractionRecordSize,
        "The interaction of the action must hold the records the engine builds.");

    // Ticks a weapon request is sent while the unit does not hold the slot (half a second at 60 ticks per second).
    constexpr std::uint32_t k_WeaponRequestTicks{ 30 };

    // Use test (A9 M5d), in ticks of a cycle: use is down until the end tick, then released, so the engine sees new presses, long
    // holds and releases.
    constexpr std::uint64_t k_UseTestCycle{ 64 };
    constexpr std::uint64_t k_UseTestDownEndTick{ 60 };

    // Sanity limit of the hold threshold read from the engine. Anything outside (0, limit) is not a threshold.
    constexpr float k_MaxHoldThresholdSeconds{ 5.0f };

    using Platform::Input::Type::k_ShadowLogLimit;

    // Weapon slot of the action that asks for nothing.
    constexpr std::uint8_t k_NoWeaponRequest{ 0xFF };

    // Milliseconds per second. The local client keeps the hold time and the threshold in whole milliseconds (0x5df80).
    constexpr std::uint32_t k_MillisecondsPerSecond{ 1000 };

    // return: True if use is down in the use test at that tick.
    auto IsUseTestDown(std::uint64_t tick) -> bool
    {
        return (tick % k_UseTestCycle) < k_UseTestDownEndTick;
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
            for (auto& ticks : m_LaneTicks) ticks.fill(0);
            return;
        }

        std::array<std::optional<ActionOverride>, k_MaxActionPlayers> overrides{};
        m_PlayerActionStore.CopyOverrides(overrides);

        std::uint32_t mask{ 0 };
        std::memcpy(&mask, update + k_UpdateActionMaskOffset, sizeof(mask));

        // note: Players with an action from the engine this tick, before the overrides are added.
        const std::uint32_t engineMask{ mask };

        // note: The tag is only read while some agent or (for the shadow check) some local player plays, never in menus or loading
        // screens.
        const bool hasOverride{ std::ranges::any_of(overrides, [](const std::optional<ActionOverride>& value) {
            return value.has_value();
        }) };
        const bool isShadowEnabled{ m_PlayerActionStore.IsShadowEnabled() };
        const bool isTagNeeded{ hasOverride || (isShadowEnabled && engineMask != 0) };
        const std::optional<std::uint32_t> holdTicks{ isTagNeeded ? this->GetHoldThresholdTicks() : std::nullopt };

        if (isShadowEnabled)
        {
            this->CheckShadow(update, threadContext, engineMask, overrides, holdTicks);
        }
        else
        {
            m_ShadowPlayers.fill(ShadowPlayer{});
        }

        const std::uintptr_t controlGlobals{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ControlGlobalsSlot) };

        for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player)
        {
            const auto& actionOverride{ overrides[player] };
            if (!actionOverride)
            {
                m_WeaponRequests[player].reset();
                m_LaneTicks[player].fill(0);
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

            std::uint32_t lanes{ actionOverride->Lanes };
            if (actionOverride->IsUseTest && IsUseTestDown(m_Sequence)) lanes |= k_UseLane;

            this->WriteLanes(action, player, lanes, holdTicks);

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

    auto PlayerActionDetour::CheckShadow(const std::uint8_t* update, std::uintptr_t threadContext, std::uint32_t engineMask,
        const std::array<std::optional<ActionOverride>, k_MaxActionPlayers>& overrides, std::optional<std::uint32_t> holdTicks)
        -> void
    {
        for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player)
        {
            ShadowPlayer& shadow{ m_ShadowPlayers[player] };

            const std::optional<UnitClientState> state{ (overrides[player] || (engineMask & (1u << player)) == 0)
                ? std::nullopt : m_BotService.GetUnitClientState(threadContext, player) };
            if (!state)
            {
                shadow = ShadowPlayer{};
                continue;
            }

            const std::uint8_t* action{ update + k_UpdateActionsOffset + (player * k_PlayerActionSize) };

            std::array<std::uint8_t, k_ActionInteractionSize> interaction{};
            if (m_BotService.WriteInteraction(threadContext, player, interaction))
            {
                std::uint32_t engineObject{ 0 };
                std::uint32_t artemisObject{ 0 };
                std::memcpy(&engineObject, action + k_ActionInteractionOffset + k_InteractionRecordObject, sizeof(engineObject));
                std::memcpy(&artemisObject, interaction.data() + k_InteractionRecordObject, sizeof(artemisObject));

                const bool isMatch{ std::memcmp(action + k_ActionInteractionOffset, interaction.data(), interaction.size()) == 0 };
                const bool isLate{ !isMatch && shadow.HasPreviousInteraction && std::memcmp(action + k_ActionInteractionOffset,
                    shadow.PreviousInteraction.data(), shadow.PreviousInteraction.size()) == 0 };

                if (isLate) this->ReportShadow(ShadowField::InteractionLate, player, false, engineObject, artemisObject);
                else        this->ReportShadow(ShadowField::Interaction, player, isMatch, engineObject, artemisObject);

                std::memcpy(shadow.PreviousInteraction.data(), interaction.data(), interaction.size());
                shadow.HasPreviousInteraction = true;
            }

            std::uint32_t lanes{ 0 };
            std::memcpy(&lanes, action + k_ActionLanesOffset, sizeof(lanes));

            for (std::size_t lane{ 0 }; lane < k_HoldLanes.size(); ++lane)
            {
                std::uint32_t& ticks{ shadow.LaneTicks[lane] };

                if ((lanes & (1u << k_HoldLanes[lane].DownBit)) == 0)
                {
                    ticks = 0;
                    shadow.IsLaneSeenUp[lane] = true;
                    continue;
                }

                if (ticks < (std::numeric_limits<std::uint32_t>::max)()) ++ticks;

                // note: A lane already down when the check started has an unknown count.
                if (!holdTicks || !shadow.IsLaneSeenUp[lane]) continue;

                const bool isEngineHeld{ (lanes & (1u << k_HoldLanes[lane].HeldBit)) != 0 };
                const bool isArtemisHeld{ ticks >= *holdTicks };
                const bool isNearThreshold{ ticks + 1 >= *holdTicks && ticks <= *holdTicks + 1 };
                this->ReportShadow(ShadowField::HeldLane, player, isEngineHeld == isArtemisHeld || isNearThreshold, isEngineHeld,
                    isArtemisHeld);
            }

            const std::uint8_t slot{ action[k_ActionWeaponSlotOffset] };
            std::uint16_t echo{ 0 };
            std::memcpy(&echo, action + k_ActionInventoryVersionOffset, sizeof(echo));
            // note: Checked once per request, on the tick it changes. A press while the previous request is still pending goes on
            // from that one, as a double tap does.
            const std::uint8_t previousSlot{ shadow.WeaponSlot };
            shadow.WeaponSlot = slot;
            if (slot != previousSlot && slot != k_NoWeaponRequest && static_cast<std::int8_t>(slot) != state->WeaponSlot
                && echo == state->InventoryVersion)
            {
                UnitClientState from{ *state };
                if (previousSlot != k_NoWeaponRequest && static_cast<std::int8_t>(previousSlot) != state->WeaponSlot)
                {
                    from.WeaponSlot = static_cast<std::int8_t>(previousSlot);
                }

                const std::optional<std::int8_t> expected{ m_BotService.GetNextWeaponSlot(from) };
                this->ReportShadow(ShadowField::Weapon, player, expected == static_cast<std::int8_t>(slot), slot,
                    expected.value_or(-1));
            }

            std::uint16_t grenade{ 0 };
            std::memcpy(&grenade, action + k_ActionGrenadeTypeOffset, sizeof(grenade));
            const std::uint16_t previousGrenade{ shadow.GrenadeType };
            shadow.GrenadeType = grenade;
            if (grenade != previousGrenade && grenade != 0xFFFF && static_cast<std::int8_t>(grenade) != state->GrenadeType)
            {
                UnitClientState from{ *state };
                if (previousGrenade != 0xFFFF && static_cast<std::int8_t>(previousGrenade) != state->GrenadeType)
                {
                    from.GrenadeType = static_cast<std::int8_t>(previousGrenade);
                }

                const std::optional<std::int8_t> expected{ m_BotService.GetNextGrenadeType(from) };
                this->ReportShadow(ShadowField::Grenade, player, expected == static_cast<std::int8_t>(grenade), grenade,
                    expected.value_or(-1));
            }

            std::int16_t zoom{ 0 };
            std::memcpy(&zoom, action + k_ActionZoomLevelOffset, sizeof(zoom));
            if (zoom > k_NoZoomLevel && zoom != state->ZoomLevel)
            {
                const std::optional<std::int8_t> expected{ m_BotService.GetNextZoomLevel(*state) };
                this->ReportShadow(ShadowField::Zoom, player, expected == static_cast<std::int8_t>(zoom), zoom,
                    expected.value_or(-1));
            }
        }
    }

    auto PlayerActionDetour::ReportShadow(ShadowField field, std::size_t player, bool isMatch, std::int64_t engineValue,
        std::int64_t artemisValue) -> void
    {
        const std::uint64_t mismatched{ m_PlayerActionStore.AddShadow(field, isMatch) };
        if (isMatch || mismatched > k_ShadowLogLimit) return;

        m_LogsService.Message("[PlayerActionDetour] WARNING: Shadow check: {} of player {} differs (engine {:X}, Artemis {:X}).",
            k_ShadowFieldNames[static_cast<std::size_t>(field)], player, engineValue, artemisValue);
    }

    auto PlayerActionDetour::WriteLanes(std::uint8_t* action, std::size_t player, std::uint32_t lanes,
        std::optional<std::uint32_t> holdTicks) -> void
    {
        auto& ticks{ m_LaneTicks[player] };

        for (std::size_t lane{ 0 }; lane < k_HoldLanes.size(); ++lane)
        {
            if ((lanes & (1u << k_HoldLanes[lane].DownBit)) == 0)
            {
                ticks[lane] = 0;
                continue;
            }

            if (ticks[lane] < (std::numeric_limits<std::uint32_t>::max)()) ++ticks[lane];
            if (holdTicks && ticks[lane] >= *holdTicks) lanes |= 1u << k_HoldLanes[lane].HeldBit;
        }

        std::memcpy(action + k_ActionLanesOffset, &lanes, sizeof(lanes));
    }

    auto PlayerActionDetour::GetHoldThresholdTicks() const -> std::optional<std::uint32_t>
    {
        if (!m_PlayerControlTable || !m_ResolveTagAddress) return std::nullopt;

        const std::uintptr_t table{ *m_PlayerControlTable };
        if (table == 0) return std::nullopt;

        // note: Between maps the address can be empty. Resolving it would give a pointer to nothing.
        const auto* address{ reinterpret_cast<const std::uint32_t*>(table + k_PlayerControlTagOffset) };
        if (*address == 0 || *address == 0xFFFFFFFF) return std::nullopt;

        const std::uintptr_t tag{ m_ResolveTagAddress(address) };
        if (tag == 0) return std::nullopt;

        const float seconds{ *reinterpret_cast<const float*>(tag + k_HoldThresholdOffset) };
        if (!(seconds > 0.0f && seconds < k_MaxHoldThresholdSeconds)) return std::nullopt;

        // note: The local client compares whole milliseconds held with (int)(seconds * 1000), so a lane is held from the first tick
        // whose time reaches it.
        const auto milliseconds{ static_cast<std::uint32_t>(seconds * static_cast<float>(k_MillisecondsPerSecond)) };
        const auto ticksPerSecond{ static_cast<std::uint32_t>(k_TicksPerSecond) };
        return ((milliseconds * ticksPerSecond) + (k_MillisecondsPerSecond - 1)) / k_MillisecondsPerSecond;
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

        return Platform::Hook::System::ReadThreadContext(*m_EngineTlsIndex);
    }

    auto PlayerActionDetour::FindEngineTlsIndex() -> const std::uint32_t*
    {
        return Platform::Hook::System::ResolveTlsIndex(m_AOBService.FindPattern(Signature::k_TelemetryIdModifier));
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

        m_ClearPlayerAction = reinterpret_cast<ClearPlayerActionFunction>(m_AOBService.FindPattern(Signature::k_ClearPlayerAction));
        if (!m_ClearPlayerAction)
        {
            m_LogsService.Message("[PlayerActionDetour] ERROR: Failed to find ClearPlayerAction. No action override will be"
                " applied.");
        }

        const std::uintptr_t tagLoad{ m_AOBService.FindPattern(Signature::k_PlayerControlTagLoad) };
        if (tagLoad != 0)
        {
            m_PlayerControlTable = reinterpret_cast<const std::uintptr_t*>(
                ResolveRelative(tagLoad, k_TagLoadTableRelOffset, k_TagLoadTableNextOffset));
            m_ResolveTagAddress = reinterpret_cast<ResolveTagAddressFunction>(
                ResolveRelative(tagLoad, k_TagLoadCallRelOffset, k_TagLoadCallNextOffset));
        }
        else
        {
            m_LogsService.Message("[PlayerActionDetour] ERROR: Failed to find PlayerControlTagLoad. Held buttons of the agents will"
                " never count as held.");
        }

        m_EngineTlsIndex = this->FindEngineTlsIndex();
        if (!m_EngineTlsIndex)
        {
            m_LogsService.Message("[PlayerActionDetour] WARNING: Failed to find the engine thread context. No action override"
                " will be applied and no bot can be added.");
        }

        void* functionAddress{ reinterpret_cast<void*>(m_AOBService.FindPattern(Signature::k_ApplyPlayerActions)) };
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
        Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[PlayerActionDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}