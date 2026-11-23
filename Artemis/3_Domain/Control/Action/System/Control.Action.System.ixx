export module Control.Action.System;

import Control.Action.Type;
import Control.Client.Type;
import Blam.Layout.Action.Type;
import std;

namespace
{
    using Control::Action::Type::ActionOverride;
    using Control::Action::Type::ClientRequests;
    using Control::Action::Type::ClientAction;
    using Control::Action::Type::PlayerActionHistory;
    using Control::Action::Type::UnitClientState;
    using Control::Client::Type::ClientContextSource;
    using Control::Client::Type::ClientOutput;
    using Control::Client::Type::ClientSettings;
    using Control::Client::Type::UnitContext;

    using Blam::Layout::Action::Type::Size::k_PlayerActionSize;
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

        // return: Whether the unit can zoom now (0x4852a0). The zoom steps ask for nothing when it cannot.
        virtual auto CanZoom(const UnitClientState& state) const -> bool = 0;

        // return: What the client asks of the unit in the state (D356, VW.9c.2): the engine facts the pure client needs, asked of that
        // unit and not of the unit of a local player. UnitContext::Known has the facts it resolved.
        // param source: The user-level ids and the client zoom (see ClientContextSource).
        // param settings: The settings of the client (the ticks per second set the jump assist limit).
        // param deltaTime: Seconds of the call (the look offset function takes them), the same dt StepClient gets.
        // note: A fact the engine side cannot resolve stays outside Known. Game thread only.
        virtual auto GetClientContext(const UnitClientState& state, const ClientContextSource& source, const ClientSettings& settings,
            float deltaTime) const -> UnitContext = 0;
    };

    // Converts the hold threshold of the player control tag to ticks.
    // param seconds: Hold threshold read from the tag.
    // return: Ticks to hold a lane, or nullopt if the value is not a threshold (outside 0 to a sanity limit, or not a number).
    auto GetHoldThresholdTicks(float seconds) -> std::optional<std::uint32_t>;

    // Turns the output of the pure client into the override and the requests of a possessed bot (VW.9c.2 B, D354): the same override the
    // agents of Python use, so BuildAction writes the action (buttons, lanes with their held bits, weapon, grenade and zoom requests).
    // param output: What StepClient left. Its look is already in yaw and pitch.
    // param yaw: Absolute facing the bot must have (the look deltas already added to the aim of the bot), radians.
    // param pitch: Absolute pitch, radians.
    // return: The buttons below bit 48, the lanes without held bits (BuildAction counts them), the move axes, the triggers and the events word
    // of the reload and the grenade throw. The events of the output become requests: switch weapon,
    // switch grenade and zoom (press, step in and step out, each its own request).
    auto MakeClientAction(const ClientOutput& output, float yaw, float pitch) -> ClientAction;

    // return: The override of a player that does nothing: no buttons, no throttle, no lanes, looking where its unit looks (the facing of the
    // state, or zero without a unit).
    auto MakeNeutralOverride(const std::optional<UnitClientState>& unit) -> ActionOverride;

    // Writes the fields of an agent into its action: facing, buttons, throttle, hold lanes and client requests (D109, D119-D121).
    // param action: Action of the player, already cleared by the engine and with the slot of its unit.
    // param unit: Client state of its unit, or nullopt without a live unit (then no request is sent).
    // param holdTicks: Hold threshold in ticks, or nullopt if unknown (then no held bit is set).
    // param sequence: Action tick of the use test.
    // param history: What the action of this agent remembers between ticks. Updated.
    // return: The facing written, which the caller gives to the control block of a local player.
    // note: Same bytes every tick for the same inputs and history. Only its own fields are written.
    auto BuildAction(std::span<std::uint8_t, k_PlayerActionSize> action, const ActionOverride& actionOverride,
        const std::optional<UnitClientState>& unit, std::optional<std::uint32_t> holdTicks, const ClientRequests& requests,
        std::uint64_t sequence, const UnitRules& rules, PlayerActionHistory& history) -> std::array<float, 2>;
}