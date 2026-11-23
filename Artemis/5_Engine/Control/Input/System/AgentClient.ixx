export module Engine.Control.Input.System:AgentClient;

import :Possession;

import Common.Engine.Type;
import Control.Action.Type;
import Control.Client.Type;
import Control.Client.System;
import Engine.Control.Input.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;

    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::System::MakeAgentFrame;
    using ::Control::Client::Type::AgentInput;
    using ::Control::Client::Type::AgentInputState;
    using ::Control::Client::Type::ClientSettings;
    using ::Control::Client::Type::ClientState;
    using ::Control::Client::Type::InputFrame;

    using Engine::Control::Input::Type::AgentClient;
    using Engine::Control::Input::Type::AgentDelivery;
    using Engine::Control::Input::Type::AgentPlan;
    using Engine::Control::Input::Type::ClientDriven;
}

export namespace Engine::Control::Input::System
{
    // return: What the tick does with the AgentInput the store holds for the player. A delivery for another player than the one in the index (its handle
    // is not the current one, or the slot is empty) is Stale. Possession wins (D351): the possessed player is Suspend, and its AgentInput waits in the
    // store until the possession ends. Any other is Run.
    // param currentHandle: Handle of the player in the index now, nothing when the slot is empty.
    inline auto PlanAgent(const AgentDelivery& delivery, std::size_t playerIndex, std::optional<std::size_t> possessed,
        std::optional<std::uint32_t> currentHandle) -> AgentPlan
    {
        if (!currentHandle || *currentHandle != delivery.Handle) return AgentPlan::Stale;

        return playerIndex == possessed ? AgentPlan::Suspend : AgentPlan::Run;
    }

    // Moves the tick of an agent inside its window (VW.9c.3 C): a delivery it did not see before starts a window at tick 0, any other tick is one more.
    // return: The tick inside the window. It reaches the window length and goes on counting when no new delivery comes (saturating).
    // param sequence: The sequence number of the delivery the store holds now.
    inline auto AdvanceAgentWindow(AgentClient& agent, std::uint64_t sequence) -> std::uint32_t
    {
        if (!agent.HasSequence || agent.Sequence != sequence)
        {
            agent.HasSequence = true;
            agent.Sequence = sequence;
            agent.TickInWindow = 0;
        }
        else if (agent.TickInWindow < (std::numeric_limits<std::uint32_t>::max)())
        {
            ++agent.TickInWindow;
        }

        return agent.TickInWindow;
    }

    // return: The AgentInput the tick plays. Inside the window of the delivery it is the delivery. Past it (no new delivery came) it is the same held
    // controls and move axes with no press and no look: what is held goes on, the axes go on, a tap is over and nothing turns.
    inline auto GetAgentTickInput(const AgentDelivery& delivery, std::uint32_t tickInWindow) -> AgentInput
    {
        if (tickInWindow < delivery.WindowTicks) return delivery.Input;

        AgentInput repeated{ delivery.Input };
        repeated.Edge = 0;
        repeated.YawDelta = 0.0f;
        repeated.PitchDelta = 0.0f;
        return repeated;
    }

    // return: The keyboard and mouse frame of the tick, from the synthesizer of the agent (MakeAgentFrame).
    // param tickInWindow: What AdvanceAgentWindow returned for this tick.
    inline auto MakeAgentTickFrame(AgentClient& agent, const AgentDelivery& delivery, std::uint32_t tickInWindow, const ClientSettings& settings)
        -> InputFrame
    {
        constexpr std::uint32_t maxTicks{ static_cast<std::uint32_t>((std::numeric_limits<std::int32_t>::max)()) };

        return MakeAgentFrame(GetAgentTickInput(delivery, tickInWindow), static_cast<std::int32_t>((std::min)(tickInWindow, maxTicks)),
            static_cast<std::int32_t>((std::min)(delivery.WindowTicks, maxTicks)), settings, delivery.TurnLimit, agent.Input);
    }

    // Gets the client state of the agent ready for the step of a tick: a new unit (a respawn) starts the aim from the unit, forgets what the last unit
    // asked for and forgets the controls the synthesizer held (a new life presses again), and the zoom is the one of the unit. An agent that a possession
    // suspended on the same unit keeps them.
    // return: The state to step.
    inline auto PrepareAgentFrame(AgentClient& agent, const UnitClientState& unit) -> ClientState
    {
        // note: The unit a possession suspended the agent with (ResumedUnit) is the same life: the held controls stay and are not pressed again (D367).
        const std::uint32_t resumedUnit{ std::exchange(agent.ResumedUnit, k_NoHandle) };
        if (agent.Client.Unit != unit.Unit && resumedUnit != unit.Unit) agent.Input = AgentInputState{};

        return PrepareFrame(agent.Client, unit);
    }

    // Forgets what the agent held for its unit, when the unit is gone. The window and the last delivery it saw stay, so a respawn in the middle of a window
    // does not play the press of its start again.
    inline auto ResetAgentUnit(AgentClient& agent) -> void
    {
        agent.Client = ClientDriven{ .Player = agent.Client.Player, .Handle = agent.Client.Handle };
        agent.Input = AgentInputState{};
    }

    // Takes back the synthesizer after a tick whose step did not run (VW.9c.3 D): what the controls counted on that tick is not kept, so a held control is
    // not counted as pressed on a tick nobody played. The start mask of the window stays, because the first tick of the window has already fixed it.
    // note: The window itself goes on counting. A tap whose tick did not run is lost, it does not move to the next tick.
    // param before: The state of the synthesizer before the frame of the tick was made.
    inline auto UndoAgentFrame(AgentClient& agent, const AgentInputState& before) -> void
    {
        const std::uint32_t startMask{ agent.Input.StartMask };
        agent.Input = before;
        agent.Input.StartMask = startMask;
    }
}