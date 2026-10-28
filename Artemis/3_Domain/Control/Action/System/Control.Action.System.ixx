export module Control.Action.System;

import Control.Action.Type;
import Layout.Action.Type;
import std;

namespace
{
    using Control::Action::Type::ActionOverride;
    using Control::Action::Type::ClientRequests;
    using Control::Action::Type::PlayerActionHistory;
    using Control::Action::Type::UnitClientState;

    using Layout::Action::Type::Size::k_PlayerActionSize;
}

export namespace Control::Action::System
{
    // Questions about a unit that only the engine can answer (D213): the next weapon, grenade and zoom level a client would ask for.
    // note: The bot service implements it with the engine functions. Game thread only.
    class UnitRules
    {
    public:
        virtual ~UnitRules() = default;

        // return: The slot the engine picks when the player switches weapon from the one in the state, or nullopt if there is none.
        virtual auto GetNextWeaponSlot(const UnitClientState& state) const -> std::optional<std::int8_t> = 0;

        // return: The next grenade type with grenades left, or nullopt if there is no other one.
        virtual auto GetNextGrenadeType(const UnitClientState& state) const -> std::optional<std::int8_t> = 0;

        // return: The next zoom level of the weapon in the hands (-1 = not zoomed), or nullopt if the unit cannot zoom now.
        virtual auto GetNextZoomLevel(const UnitClientState& state) const -> std::optional<std::int8_t> = 0;
    };

    // Converts the hold threshold of the player control tag to ticks.
    // param seconds: Hold threshold read from the tag.
    // return: Ticks to hold a lane, or nullopt if the value is not a threshold (outside 0 to a sanity limit, or not a number).
    auto GetHoldThresholdTicks(float seconds) -> std::optional<std::uint32_t>;

    // Writes the fields of an agent into its action: facing, buttons, throttle, hold lanes and client requests (D109, D119-D121).
    // param action: Action of the player, already cleared by the engine and with the slot of its unit.
    // param unit: Client state of its unit, or nullopt without a live unit (then the facing goes straight to the target and no
    // request is sent).
    // param holdTicks: Hold threshold in ticks, or nullopt if unknown (then no held bit is set).
    // param sequence: Action tick of the use test.
    // param history: What the action of this agent remembers between ticks. Updated.
    // return: The facing written, which the caller gives to the control block of a local player.
    // note: Same bytes every tick for the same inputs and history. Only its own fields are written.
    auto BuildAction(std::span<std::uint8_t, k_PlayerActionSize> action, const ActionOverride& actionOverride,
        const std::optional<UnitClientState>& unit, std::optional<std::uint32_t> holdTicks, const ClientRequests& requests,
        std::uint64_t sequence, const UnitRules& rules, PlayerActionHistory& history) -> std::array<float, 2>;
}