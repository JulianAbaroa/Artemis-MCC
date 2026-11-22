module;

#include <windows.h>

module Engine.Control.Input.Hook;
import :PlayerAction;

import Common.Engine.Type;
import Control.Action.Type;
import Control.Action.System;
import Engine.Base.Hook.System;
import Engine.Base.Memory.Type;
import Engine.Control.Input.System;
import Engine.Control.Simulation.Type;
import Blam.Layout.Action.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Player.Type;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;

    using ::Control::Action::System::BuildAction;

    using Engine::Base::Hook::System::k_DrainTimeout;

    using ::Control::Action::System::MakeNeutralOverride;
    using ::Control::Action::Type::PossessedAction;

    using Engine::Control::Input::Type::PossessedBot;
    using Engine::Control::Input::Type::k_PossessInputUser;
    using Engine::Control::Input::Type::k_PossessRequestMaxAge;
    using Engine::Control::Input::System::ShouldNeutralizeLocal;
    using Engine::Control::Input::System::TakeTickAction;
    using Engine::Control::Input::System::TickFrameAge;
    using Engine::Control::Input::System::FindInputPlayer;
    using Engine::Control::Input::System::FindPossessedBot;
    using Engine::Control::Input::System::GetEngineThreadContext;
    using Engine::Control::Input::System::LocateControlTag;
    using Engine::Control::Input::System::ReadControlTag;

    using ::Control::Action::Type::ActionOverride;
    using ::Control::Action::Type::ClientRequests;
    using ::Control::Action::Type::PlayerActionHistory;
    using ::Control::Action::Type::UnitClientState;

    using Common::Engine::Type::k_MaxLocalUsers;

    using Engine::Control::Input::System::FindViewActionPlayer;

    using Engine::Control::Input::Type::PlayerActions;
    using Engine::Control::Input::Type::ShadowField;

    using Blam::Layout::Object::Type::Constant::k_GameGlobalsTheaterOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextGameModeOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextMappingOffset;
    using Blam::Layout::Player::Type::Mapping::k_EntryStride;
    using Blam::Layout::Player::Type::Mapping::k_OutputUserPlayer;
    using Blam::Layout::Player::Type::Mapping::k_PlayerInputUser;

    using Blam::Layout::Action::Type::k_HoldLanes;
    using Blam::Layout::Action::Type::k_NoZoomLevel;
    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
    using Blam::Layout::Action::Type::Size::k_PlayerActionSize;
    using Blam::Layout::Action::Type::Size::k_ActionInteractionSize;
    using Blam::Layout::Action::Type::Offset::k_UpdateActionMaskOffset;
    using Blam::Layout::Action::Type::Offset::k_UpdateActionsOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionHasUnitOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionUnitSlotOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionLanesOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionInteractionOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionInventoryVersionOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionWeaponSlotOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionGrenadeTypeOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionZoomLevelOffset;

    using Engine::Control::Input::Type::k_HoldThresholdOffset;
    using Engine::Control::Input::Type::k_ShadowFieldNames;
    using Engine::Control::Input::Type::k_ControlGlobalsSlot;
    using Engine::Control::Input::Type::k_ControlBlockStride;
    using Engine::Control::Input::Type::k_ControlFacingOffset;

    using Engine::Control::Simulation::Type::k_InteractionRecordCount;
    using Engine::Control::Simulation::Type::k_InteractionRecordSize;
    using Engine::Control::Simulation::Type::k_InteractionRecordObject;

    static_assert(k_ActionInteractionSize == k_InteractionRecordCount * k_InteractionRecordSize,
        "The interaction of the action must hold the records the engine builds.");

    using Engine::Control::Input::Type::k_ShadowLogLimit;

    // Weapon slot of the action that asks for nothing.
    constexpr std::uint8_t k_NoWeaponRequest{ 0xFF };

    // Aims the action at the unit in that slot.
    auto WriteUnitSlot(std::uint8_t* action, std::uint8_t unitSlot) -> void
    {
        constexpr std::uint32_t hasUnit{ 1 };
        std::memcpy(action + k_ActionHasUnitOffset, &hasUnit, sizeof(hasUnit));
        action[k_ActionUnitSlotOffset] = unitSlot;
    }
}

namespace Engine::Control::Input::Hook
{
    auto __fastcall PlayerActionDetour::HookedApplyPlayerActions(void* update) -> void
    {
        Engine::Base::Hook::System::InFlightScope scope(s_InFlight);

        const std::uintptr_t threadContext{ update != nullptr ? GetEngineThreadContext(s_Instance->m_EngineTlsIndex) : 0 };

        if (update != nullptr)
        {
            auto* bytes{ static_cast<std::uint8_t*>(update) };

            s_Instance->m_MatchSetupService.UpdateLoad(threadContext);
            s_Instance->m_BotService.Update(threadContext);
            s_Instance->m_ActionSource->Prepare(s_Instance->m_Sequence + 1);
            s_Instance->ApplyOverrides(bytes, threadContext);
            s_Instance->PublishActions(bytes);
        }

        s_OriginalFunction(update);

        if (update != nullptr)
        {
            s_Instance->CopyActionsToViews(static_cast<const std::uint8_t*>(update), threadContext);
        }
    }

    auto PlayerActionDetour::CopyActionsToViews(const std::uint8_t* update, std::uintptr_t threadContext) -> void
    {
        if (!m_CopyActionToView || threadContext == 0) return;

        const std::uintptr_t mapping{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextMappingOffset) };
        const std::uintptr_t gameGlobals{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextGameModeOffset) };
        if (mapping == 0 || gameGlobals == 0) return;

        const bool isTheater{ *reinterpret_cast<const std::uint8_t*>(gameGlobals + k_GameGlobalsTheaterOffset) != 0 };

        // note: The mask after ApplyOverrides, the one the engine just used to apply the actions.
        std::uint32_t mask{ 0 };
        std::memcpy(&mask, update + k_UpdateActionMaskOffset, sizeof(mask));

        std::array<std::int32_t, k_MaxActionPlayers> inputUsers{};
        std::memcpy(inputUsers.data(), reinterpret_cast<const void*>(mapping + k_PlayerInputUser), sizeof(inputUsers));

        for (std::size_t view{ 0 }; view < k_MaxLocalUsers; ++view)
        {
            std::uint32_t viewPlayer{ 0 };
            std::memcpy(&viewPlayer, reinterpret_cast<const void*>(mapping + k_OutputUserPlayer + (view * k_EntryStride)),
                sizeof(viewPlayer));

            const std::optional<std::size_t> player{ FindViewActionPlayer(viewPlayer, mask, inputUsers, isTheater) };
            if (!player) continue;

            m_CopyActionToView(static_cast<std::int32_t>(view), update + k_UpdateActionsOffset + (*player * k_PlayerActionSize));
        }
    }

    auto PlayerActionDetour::SetPossession(PossessRequestStore& requestStore, PossessionStore& possessionStore) -> void
    {
        m_PossessRequestStore = &requestStore;
        m_PossessionStore = &possessionStore;
    }

    auto PlayerActionDetour::ApplyPossession(std::uintptr_t threadContext, std::uint32_t engineMask,
        std::array<std::optional<ActionOverride>, k_MaxActionPlayers>& overrides, std::array<ClientRequests, k_MaxActionPlayers>& requests)
        -> void
    {
        if (m_PossessRequestStore == nullptr || m_PossessionStore == nullptr) return;

        // note: Turned off for good when the frames ran on another thread than the ticks (PossessionService logs the ERROR).
        if (m_PossessionStore->IsDisabled())
        {
            m_PossessionStore->Release();
            return;
        }

        const std::optional<std::uint32_t> request{ m_PossessRequestStore->Read(k_PossessRequestMaxAge) };
        const std::optional<std::size_t> bot{ request ? FindPossessedBot(request, m_BotService.GetBotPlayerHandles(threadContext)) : std::nullopt };
        if (!bot)
        {
            m_PossessionStore->Release();
            return;
        }

        const std::optional<std::size_t> humanLocal{ FindInputPlayer(threadContext, k_PossessInputUser) };

        std::uint32_t overrideMask{ 0 };
        for (std::size_t player{ 0 }; player < overrides.size(); ++player)
        {
            if (overrides[player]) overrideMask |= 1u << player;
        }

        // note: The neutral action of the human local, always while a bot is possessed, with or without a unit (a dead local must not play with the keys).
        if (ShouldNeutralizeLocal(bot, humanLocal, overrideMask, engineMask))
        {
            overrides[*humanLocal] = MakeNeutralOverride(m_BotService.GetUnitClientState(threadContext, *humanLocal));
            requests[*humanLocal] = ClientRequests{};
        }

        m_PossessionStore->Begin(*bot, *request);
        PossessedBot* possessed{ m_PossessionStore->Find() };
        if (possessed == nullptr) return;

        // note: The Python action of the possessed bot is discarded (D351). The bot gets the neutral action unless the frames of the client left one.
        const std::optional<UnitClientState> unitState{ m_BotService.GetUnitClientState(threadContext, *bot) };
        overrides[*bot] = MakeNeutralOverride(unitState);
        requests[*bot] = ClientRequests{};

        if (!unitState)
        {
            m_PossessionStore->CountTick(false, false);
            if (m_PossessionStore->MarkUnitMissing())
            {
                m_Log.Message("[PlayerActionDetour] WARNING: The possessed bot (player {}) has no unit: it gets a neutral action until it spawns.",
                    *bot);
            }
            return;
        }

        // note: The client runs per frame in the 0x5df80 hook (D360). No frame since the unit began, or a unit the frames did not see, is a neutral tick.
        // note: A frame that did not run for more than k_PossessMaxStaleTicks ticks is a neutral tick too, and the held state is forgotten.
        if (possessed->Unit != unitState->Unit || TickFrameAge(*possessed))
        {
            m_PossessionStore->CountTick(false, false);
            return;
        }

        const bool isUnsupported{ (possessed->Last.Unsupported | possessed->Pending.Unsupported) != 0 };
        PossessedAction action{ TakeTickAction(*possessed) };
        overrides[*bot] = action.Override;
        requests[*bot] = action.Requests;
        m_PossessionStore->CountTick(true, isUnsupported);
    }

    auto PlayerActionDetour::ApplyOverrides(std::uint8_t* update, std::uintptr_t threadContext) -> void
    {
        // note: Taken every tick, so a press never fires late.
        std::array<ClientRequests, k_MaxActionPlayers> requests{};
        m_PlayerActionStore.TakeClientRequests(requests);

        // note: Without the thread context the unit slot is unknown, and a cleared action would leave the unit without control.
        if (!m_ClearPlayerAction || threadContext == 0)
        {
            for (auto& history : m_Histories)
            {
                history.PendingWeapon.reset();
                history.LaneTicks.fill(0);
            }
            return;
        }

        std::array<std::optional<ActionOverride>, k_MaxActionPlayers> overrides{};
        m_PlayerActionStore.CopyOverrides(overrides);

        std::uint32_t mask{ 0 };
        std::memcpy(&mask, update + k_UpdateActionMaskOffset, sizeof(mask));

        // note: Players with an action from the engine this tick, before the overrides are added.
        const std::uint32_t engineMask{ mask };

        this->ApplyPossession(threadContext, engineMask, overrides, requests);

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
                m_Histories[player] = PlayerActionHistory{};
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

            const std::optional<UnitClientState> unitState{ m_BotService.GetUnitClientState(threadContext, player) };

            // note: Writes the buttons, facing, throttle, lanes and client requests of the agent, in that order (D109, D114, D120, D121).
            // The sequence is the action tick of the use test, before PublishActions counts this tick.
            const std::array<float, 2> facing{ BuildAction(std::span<std::uint8_t, k_PlayerActionSize>{ action, k_PlayerActionSize },
                *actionOverride, unitState, holdTicks, requests[player], m_Sequence, m_BotService, m_Histories[player]) };

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

        m_Log.Message("[PlayerActionDetour] WARNING: Shadow check: {} of player {} differs (engine {:X}, Artemis {:X}).",
            k_ShadowFieldNames[static_cast<std::size_t>(field)], player, engineValue, artemisValue);
    }

    auto PlayerActionDetour::GetHoldThresholdTicks() const -> std::optional<std::uint32_t>
    {
        const std::uintptr_t tag{ ReadControlTag(m_ControlTag) };
        if (tag == 0) return std::nullopt;

        const float seconds{ *reinterpret_cast<const float*>(tag + k_HoldThresholdOffset) };
        return ::Control::Action::System::GetHoldThresholdTicks(seconds);
    }

    auto PlayerActionDetour::FindEngineTlsIndex() -> const std::uint32_t*
    {
        return Engine::Base::Hook::System::ResolveTlsIndex(m_AOBService.FindPattern(Signature::k_TelemetryIdModifier));
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

    auto PlayerActionDetour::SetActionSource(ActionSource& actionSource) -> void
    {
        m_ActionSource = &actionSource;
    }

    auto PlayerActionDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;

        if (!m_ActionSource)
        {
            m_Log.Message("[PlayerActionDetour] ERROR: no action source, hook not installed.");
            return false;
        }

        s_Instance = this;
        m_Sequence = 0;

        m_ClearPlayerAction = reinterpret_cast<ClearPlayerActionFunction>(m_AOBService.FindPattern(Signature::k_ClearPlayerAction));
        if (!m_ClearPlayerAction)
        {
            m_Log.Message("[PlayerActionDetour] ERROR: Failed to find ClearPlayerAction. No action override will be"
                " applied.");
        }

        m_CopyActionToView = reinterpret_cast<CopyActionToViewFunction>(m_AOBService.FindPattern(Signature::k_CopyActionToView));
        if (!m_CopyActionToView)
        {
            m_Log.Message("[PlayerActionDetour] ERROR: Failed to find CopyActionToView. The view of a non local player (Watch POV) will not"
                " turn.");
        }

        const std::uintptr_t tagLoad{ m_AOBService.FindPattern(Signature::k_PlayerControlTagLoad) };
        if (tagLoad != 0)
        {
            m_ControlTag = LocateControlTag(tagLoad);
        }
        else
        {
            m_Log.Message("[PlayerActionDetour] ERROR: Failed to find PlayerControlTagLoad. Held buttons of the agents will"
                " never count as held.");
        }

        m_EngineTlsIndex = this->FindEngineTlsIndex();
        if (!m_EngineTlsIndex)
        {
            m_Log.Message("[PlayerActionDetour] WARNING: Failed to find the engine thread context. No action override"
                " will be applied and no bot can be added.");
        }

        void* functionAddress{ reinterpret_cast<void*>(m_AOBService.FindPattern(Signature::k_ApplyPlayerActions)) };
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedApplyPlayerActions),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[PlayerActionDetour]", m_Log))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto PlayerActionDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());
        Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[PlayerActionDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}