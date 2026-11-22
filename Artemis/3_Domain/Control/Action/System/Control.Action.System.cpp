module Control.Action.System;

import Common.Engine.Type;
import Common.Math.System;
import Control.Action.Type;
import Control.Client.Type;
import Blam.Layout.Action.Type;
import std;

namespace
{
    using Control::Action::Type::ActionOverride;
    using Control::Action::Type::AppliedFacing;
    using Control::Action::Type::ClientRequests;
    using Control::Action::Type::PlayerActionHistory;
    using Control::Action::Type::PossessedAction;
    using Control::Client::Type::ClientOutput;
    using Control::Client::Type::k_EventReload;
    using Control::Client::Type::k_EventSwitchGrenade;
    using Control::Client::Type::k_EventSwitchWeapon;
    using Control::Client::Type::k_EventZoomPress;
    using Control::Action::Type::UnitClientState;
    using Control::Action::Type::WeaponRequest;
    using Control::Action::System::UnitRules;

    using Common::Engine::Type::k_TicksPerSecond;
    using Common::Math::System::TurnToward;
    using Common::Math::System::MoveToward;

    using Blam::Layout::Action::Type::k_HoldLanes;
    using Blam::Layout::Action::Type::Size::k_PlayerActionSize;
    using Blam::Layout::Action::Type::k_UseLane;
    using Blam::Layout::Action::Type::k_ActionKeepZoomButton;
    using Blam::Layout::Action::Type::k_ActionSeatWeaponSwitchButton;
    using Blam::Layout::Action::Type::k_NoZoomLevel;
    using Blam::Layout::Action::Type::Offset::k_ActionButtonsOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionFacingOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionThrottleOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionEventsOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionFireTriggerOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionTrigger4Offset;
    using Blam::Layout::Action::Type::Offset::k_ActionZoomTriggerOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionLanesOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionInventoryVersionOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionWeaponSlotOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionGrenadeTypeOffset;
    using Blam::Layout::Action::Type::Offset::k_ActionZoomLevelOffset;

    // The client buttons the host accepts: bits 48 to 63 of the action must be zero (A17 sec. 2).
    constexpr std::uint64_t k_ValidButtons{ (1ull << 48) - 1 };

    // Event bits of the zoom step in and out (A17 sec. 6), and the shift of the reload and throw bits to the events word of the action.
    constexpr std::uint32_t k_EventZoomIn{ 1u << 4 };
    constexpr std::uint32_t k_EventZoomOut{ 1u << 5 };
    constexpr std::uint32_t k_EventsWordShift{ 8 };
    constexpr std::uint32_t k_EventsWordMask{ 3 };

    // Ticks a weapon request is sent while the unit does not hold the slot (half a second at 60 ticks per second).
    constexpr std::uint32_t k_WeaponRequestTicks{ 30 };

    // Use test (A9 M5d), in ticks of a cycle: use is down until the end tick, then released, so the engine sees new presses, long
    // holds and releases.
    constexpr std::uint64_t k_UseTestCycle{ 64 };
    constexpr std::uint64_t k_UseTestDownEndTick{ 60 };

    // Sanity limit of the hold threshold read from the engine. Anything outside (0, limit) is not a threshold.
    constexpr float k_MaxHoldThresholdSeconds{ 5.0f };

    // Milliseconds per second. The local client keeps the hold time and the threshold in whole milliseconds (0x5df80).
    constexpr std::uint32_t k_MillisecondsPerSecond{ 1000 };

    // return: True if use is down in the use test at that tick.
    auto IsUseTestDown(std::uint64_t tick) -> bool
    {
        return (tick % k_UseTestCycle) < k_UseTestDownEndTick;
    }

    // Writes the hold lanes of the agent into its action: the down bits it holds, plus the held bit of every lane down for the
    // hold threshold of the engine (D121).
    // param holdTicks: Hold threshold in ticks, or nullopt if unknown (then no held bit is set).
    // note: Counted in ticks of game time, so the speed of the engine does not change it.
    auto WriteLanes(std::span<std::uint8_t, k_PlayerActionSize> action, std::array<std::uint32_t, k_HoldLanes.size()>& ticks,
        std::uint32_t lanes, std::optional<std::uint32_t> holdTicks) -> void
    {
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

        std::memcpy(action.data() + k_ActionLanesOffset, &lanes, sizeof(lanes));
    }

    // Writes the client requests of the agent (weapon, grenade, zoom) into its action, with the engine selectors (D120).
    // note: Without a request the action asks for nothing and keeps the zoom of the unit. For a local player with an override the
    // zoom its own input asks for is ignored. A weapon request is sent again every tick until the unit holds that slot,
    // k_WeaponRequestTicks pass or the inventory changes.
    auto WriteClientRequests(std::span<std::uint8_t, k_PlayerActionSize> action, const std::optional<UnitClientState>& unit,
        const ClientRequests& requests, const UnitRules& rules, std::optional<WeaponRequest>& weaponRequest) -> void
    {
        if (!unit)
        {
            weaponRequest.reset();
        }

        // note: A seated unit has no slot to pick: 0x61a38 raises this button for the vehicle weapon instead (the Banshee cannons and
        // bomb), and clears it on every other call, so buttons of an override never keep it raised.
        std::uint64_t seatButtons{ 0 };
        std::memcpy(&seatButtons, action.data() + k_ActionButtonsOffset, sizeof(seatButtons));
        seatButtons &= ~k_ActionSeatWeaponSwitchButton;
        if (unit && requests.IsWeaponSwitch && unit->IsSeated) seatButtons |= k_ActionSeatWeaponSwitchButton;
        std::memcpy(action.data() + k_ActionButtonsOffset, &seatButtons, sizeof(seatButtons));

        if (!unit) return;

        const UnitClientState& state{ *unit };

        if (weaponRequest && (state.WeaponSlot == weaponRequest->Slot || weaponRequest->TicksLeft == 0
            || state.InventoryVersion != weaponRequest->InventoryVersion))
        {
            weaponRequest.reset();
        }

        if (requests.IsWeaponSwitch && !state.IsSeated)
        {
            // note: A second press while a request is pending goes on from the requested slot, as a double tap does.
            UnitClientState from{ state };
            if (weaponRequest) from.WeaponSlot = weaponRequest->Slot;

            const std::optional<std::int8_t> slot{ rules.GetNextWeaponSlot(from) };
            if (slot)
            {
                weaponRequest = WeaponRequest{ .Slot = *slot, .InventoryVersion = state.InventoryVersion,
                    .TicksLeft = k_WeaponRequestTicks };
            }
        }

        std::optional<std::int8_t> zoomLevel{};

        if (weaponRequest)
        {
            --weaponRequest->TicksLeft;
            std::memcpy(action.data() + k_ActionInventoryVersionOffset, &weaponRequest->InventoryVersion,
                sizeof(weaponRequest->InventoryVersion));
            action[k_ActionWeaponSlotOffset] = static_cast<std::uint8_t>(weaponRequest->Slot);

            // note: A local client leaves the zoom when it switches weapon (0x61a38). A zoom press during the switch is dropped.
            zoomLevel = k_NoZoomLevel;
        }
        else if (requests.IsZoom)
        {
            zoomLevel = rules.GetNextZoomLevel(state);
        }
        else if ((requests.IsZoomIn || requests.IsZoomOut) && rules.CanZoom(state))
        {
            // note: 0x61a38 steps from the zoom of the unit: in takes the larger of the level and the next one (the last level stays),
            // out takes one below and never goes under no zoom.
            if (requests.IsZoomIn)
            {
                const std::optional<std::int8_t> next{ rules.GetNextZoomLevel(state) };
                if (next) zoomLevel = (std::max)(state.ZoomLevel, *next);
            }
            else
            {
                zoomLevel = static_cast<std::int8_t>((std::max)(state.ZoomLevel - 1, static_cast<int>(k_NoZoomLevel)));
            }
        }

        if (requests.IsGrenadeSwitch)
        {
            const std::optional<std::int8_t> type{ rules.GetNextGrenadeType(state) };
            if (type)
            {
                const auto value{ static_cast<std::uint16_t>(*type) };
                std::memcpy(action.data() + k_ActionGrenadeTypeOffset, &value, sizeof(value));
            }
        }

        if (zoomLevel)
        {
            const auto value{ static_cast<std::uint16_t>(static_cast<std::int16_t>(*zoomLevel)) };
            std::memcpy(action.data() + k_ActionZoomLevelOffset, &value, sizeof(value));

            std::uint64_t buttons{ 0 };
            std::memcpy(&buttons, action.data() + k_ActionButtonsOffset, sizeof(buttons));
            buttons &= ~k_ActionKeepZoomButton;
            std::memcpy(action.data() + k_ActionButtonsOffset, &buttons, sizeof(buttons));
        }
    }
}

namespace Control::Action::System
{
    auto GetHoldThresholdTicks(float seconds) -> std::optional<std::uint32_t>
    {
        if (!(seconds > 0.0f && seconds < k_MaxHoldThresholdSeconds)) return std::nullopt;

        // note: The local client compares whole milliseconds held with (int)(seconds * 1000), so a lane is held from the first tick
        // whose time reaches it.
        const auto milliseconds{ static_cast<std::uint32_t>(seconds * static_cast<float>(k_MillisecondsPerSecond)) };
        const auto ticksPerSecond{ static_cast<std::uint32_t>(k_TicksPerSecond) };
        return ((milliseconds * ticksPerSecond) + (k_MillisecondsPerSecond - 1)) / k_MillisecondsPerSecond;
    }

    auto MakePossessedAction(const ClientOutput& output, float yaw, float pitch) -> PossessedAction
    {
        std::uint32_t heldBits{ 0 };
        for (const auto& lane : k_HoldLanes) heldBits |= 1u << lane.HeldBit;

        PossessedAction action{};
        ActionOverride& actionOverride{ action.Override };
        actionOverride.Buttons = output.Buttons & k_ValidButtons;
        actionOverride.Yaw = yaw;
        actionOverride.Pitch = pitch;
        actionOverride.TurnLimit = 0.0f;
        actionOverride.ThrottleForward = output.MoveForward;
        actionOverride.ThrottleLeft = output.MoveSide;
        actionOverride.Lanes = output.Lanes & ~heldBits;
        actionOverride.ClientEvents = static_cast<std::uint16_t>((output.Events >> k_EventsWordShift) & k_EventsWordMask);
        actionOverride.FireTrigger = output.FireTrigger;
        actionOverride.ZoomTrigger = output.ZoomTrigger;
        actionOverride.Trigger4 = output.Trigger4;

        action.Requests.IsWeaponSwitch = (output.Events & k_EventSwitchWeapon) != 0;
        action.Requests.IsGrenadeSwitch = (output.Events & k_EventSwitchGrenade) != 0;
        action.Requests.IsZoom = (output.Events & k_EventZoomPress) != 0;
        action.Requests.IsZoomIn = (output.Events & k_EventZoomIn) != 0;
        action.Requests.IsZoomOut = (output.Events & k_EventZoomOut) != 0;
        return action;
    }

    auto MakeNeutralOverride(const std::optional<UnitClientState>& unit) -> ActionOverride
    {
        ActionOverride actionOverride{};
        if (unit)
        {
            actionOverride.Yaw = unit->Facing[0];
            actionOverride.Pitch = unit->Facing[1];
        }

        return actionOverride;
    }

    auto BuildAction(std::span<std::uint8_t, k_PlayerActionSize> action, const ActionOverride& actionOverride,
        const std::optional<UnitClientState>& unit, std::optional<std::uint32_t> holdTicks, const ClientRequests& requests,
        std::uint64_t sequence, const UnitRules& rules, PlayerActionHistory& history) -> std::array<float, 2>
    {
        // note: The turn goes on from the facing written last tick while the unit is the same. A new unit (first tick of an
        // override, a spawn, another player in the slot, a new match) starts from the aim the unit has, so it never jumps.
        const auto& previous{ history.Facing };
        std::array<float, 2> facing{ actionOverride.Yaw, actionOverride.Pitch };
        if (unit)
        {
            const std::array<float, 2> from{ previous && previous->Unit == unit->Unit ? previous->Facing : unit->Facing };
            facing = { TurnToward(from[0], actionOverride.Yaw, actionOverride.TurnLimit),
                MoveToward(from[1], actionOverride.Pitch, actionOverride.TurnLimit) };
            history.Facing = AppliedFacing{ .Unit = unit->Unit, .Facing = facing };
        }
        else
        {
            history.Facing.reset();
        }
        const std::array<float, 2> throttle{ actionOverride.ThrottleForward, actionOverride.ThrottleLeft };

        // note: The zoom level of the action is marked absent unless the agent asks for one, so the unit keeps its zoom (D120).
        const std::uint64_t buttons{ actionOverride.Buttons | k_ActionKeepZoomButton };

        std::memcpy(action.data() + k_ActionButtonsOffset, &buttons, sizeof(buttons));
        std::memcpy(action.data() + k_ActionFacingOffset, facing.data(), sizeof(facing));
        std::memcpy(action.data() + k_ActionThrottleOffset, throttle.data(), sizeof(throttle));

        std::uint32_t lanes{ actionOverride.Lanes };
        if (actionOverride.IsUseTest && IsUseTestDown(sequence)) lanes |= k_UseLane;

        WriteLanes(action, history.LaneTicks, lanes, holdTicks);

        // note: Only the possessed bot writes these, the empty action of the engine has zero in all of them.
        if (actionOverride.ClientEvents != 0) std::memcpy(action.data() + k_ActionEventsOffset, &actionOverride.ClientEvents, sizeof(actionOverride.ClientEvents));
        if (actionOverride.FireTrigger != 0.0f) std::memcpy(action.data() + k_ActionFireTriggerOffset, &actionOverride.FireTrigger, sizeof(float));
        if (actionOverride.ZoomTrigger != 0.0f) std::memcpy(action.data() + k_ActionZoomTriggerOffset, &actionOverride.ZoomTrigger, sizeof(float));
        if (actionOverride.Trigger4 != 0.0f) std::memcpy(action.data() + k_ActionTrigger4Offset, &actionOverride.Trigger4, sizeof(float));

        WriteClientRequests(action, unit, requests, rules, history.PendingWeapon);

        return facing;
    }
}