export module Engine.Control.Input.System:Possession;

import Common.Engine.Type;
import Control.Action.Type;
import Control.Client.Type;
import Control.Action.System;
import Control.Client.System;
import Engine.Control.Input.Type;
import Blam.Layout.Action.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;

    using ::Control::Action::System::MakePossessedAction;
    using ::Control::Action::Type::PossessedAction;
    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::System::IntegrateAim;
    using ::Control::Client::Type::ClientOutput;
    using ::Control::Client::Type::ClientState;
    using ::Control::Client::Type::ClientStep;
    using ::Control::Client::Type::UnitContext;
    using ::Control::Client::Type::k_EventPulses;

    using Engine::Control::Input::Type::ClientDriven;
    using Engine::Control::Input::Type::k_PossessMaxStaleTicks;

    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
}

export namespace Engine::Control::Input::System
{
    // return: The player index of the bot the Viewer asked for, or nothing.
    // param requested: The player handle the Viewer asked to possess, nothing when it asks none.
    // param botHandles: The handle of the Artemis bot in each player slot, k_NoHandle where there is none. Only these can be possessed: a human
    // (local or of the network) never is, and a handle whose salt changed (another bot in the slot) is not the one asked for.
    inline auto FindPossessedBot(std::optional<std::uint32_t> requested, const std::array<std::uint32_t, k_MaxActionPlayers>& botHandles)
        -> std::optional<std::size_t>
    {
        if (!requested || *requested == k_NoHandle) return std::nullopt;

        for (std::size_t player{ 0 }; player < botHandles.size(); ++player)
        {
            if (botHandles[player] == *requested) return player;
        }

        return std::nullopt;
    }

    // return: Whether the human local player of the user must get a neutral action this tick (D350): a bot is possessed, the local is another player, it
    // has an action from the engine and no override of an agent (a local that is an agent is never touched, the input of the user is not for it). It
    // does not depend on the unit: a dead local with a live bot possessed must not play with the keys of the user either.
    // param possessed: The possessed bot, nothing when none.
    // param humanLocal: The player of input user 0, nothing when there is none.
    // param overrideMask: Players with an override of an agent (bit per player index).
    // param engineMask: Players with an action from the engine this tick.
    inline auto ShouldNeutralizeLocal(std::optional<std::size_t> possessed, std::optional<std::size_t> humanLocal, std::uint32_t overrideMask,
        std::uint32_t engineMask) -> bool
    {
        if (!possessed || !humanLocal || *possessed >= k_MaxActionPlayers || *humanLocal >= k_MaxActionPlayers || *humanLocal == *possessed) return false;

        return (overrideMask & (1u << *humanLocal)) == 0 && (engineMask & (1u << *humanLocal)) != 0;
    }

    // Gets the client state of the bot ready for the step of a frame: a new unit starts the aim from the unit and forgets what the last unit
    // asked for, and the zoom is the one of the unit.
    // return: The state to step.
    inline auto PrepareFrame(ClientDriven& bot, const UnitClientState& unit) -> ClientState
    {
        if (bot.Unit != unit.Unit)
        {
            bot.State.Yaw = unit.Facing[0];
            bot.State.Pitch = unit.Facing[1];
            bot.Unit = unit.Unit;
            bot.HasFrame = false;
            bot.Last = ClientOutput{};
            bot.Pending = ClientOutput{};
        }

        ClientState state{ bot.State };
        state.ZoomLevel = unit.ZoomLevel;
        return state;
    }

    // Adds one frame of the client to the bot (D360): the look goes into the absolute aim, the output becomes the held state, and what only lasts a
    // frame (buttons, lanes, pulse events, triggers) is kept in Pending until a tick delivers it.
    inline auto AccumulateFrame(ClientDriven& bot, const ClientStep& step, const UnitContext& context) -> void
    {
        const ClientOutput& output{ step.Output };

        bot.State = IntegrateAim(step.State, output.YawDelta, output.PitchDelta, context);
        bot.Last = output;
        bot.HasFrame = true;
        bot.TicksSinceFrame = 0;

        bot.Pending.Buttons |= output.Buttons;
        bot.Pending.Lanes |= output.Lanes;
        bot.Pending.Events |= output.Events & k_EventPulses;
        bot.Pending.Unsupported |= output.Unsupported;
        bot.Pending.FireTrigger = (std::max)(bot.Pending.FireTrigger, output.FireTrigger);
        bot.Pending.ZoomTrigger = (std::max)(bot.Pending.ZoomTrigger, output.ZoomTrigger);
        bot.Pending.Trigger4 = (std::max)(bot.Pending.Trigger4, output.Trigger4);
    }

    // Counts one tick since the last frame of the client.
    // return: True when the bot has no fresh frame (none yet, or more than k_PossessMaxStaleTicks ticks without one): it then gets a neutral action and its
    // held state and pending requests are cleared, so a frame that did not run is never delivered again and again.
    inline auto TickFrameAge(ClientDriven& bot) -> bool
    {
        if (bot.TicksSinceFrame < (std::numeric_limits<std::uint32_t>::max)()) ++bot.TicksSinceFrame;

        const bool isStale{ !bot.HasFrame || bot.TicksSinceFrame > k_PossessMaxStaleTicks };
        if (isStale)
        {
            bot.Last = ClientOutput{};
            bot.Pending = ClientOutput{};
        }

        return isStale;
    }

    // return: What the tick delivers for the bot: the aim accumulated by every frame, the held state of the last frame and everything the frames since
    // the last tick asked for once. Clears what was delivered, so several ticks in one frame deliver a press once and no frame is lost.
    // note: The caller checked that a frame ran (HasFrame).
    inline auto TakeTickAction(ClientDriven& bot) -> PossessedAction
    {
        ClientOutput output{ bot.Last };
        output.Buttons |= bot.Pending.Buttons;
        output.Lanes |= bot.Pending.Lanes;
        output.Events = (bot.Last.Events & ~k_EventPulses) | bot.Pending.Events;
        output.Unsupported |= bot.Pending.Unsupported;
        output.FireTrigger = (std::max)(bot.Last.FireTrigger, bot.Pending.FireTrigger);
        output.ZoomTrigger = (std::max)(bot.Last.ZoomTrigger, bot.Pending.ZoomTrigger);
        output.Trigger4 = (std::max)(bot.Last.Trigger4, bot.Pending.Trigger4);

        // note: The held state stays for the next tick when no frame comes, the pulses of the last frame are gone once delivered.
        bot.Last.Events &= ~k_EventPulses;
        bot.Pending = ClientOutput{};

        return MakePossessedAction(output, bot.State.Yaw, bot.State.Pitch);
    }
}