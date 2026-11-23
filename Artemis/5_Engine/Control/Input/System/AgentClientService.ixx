export module Engine.Control.Input.System:AgentClientService;

import Common.Log.System;
import Control.Action.Type;
import Control.Client.Type;
import Engine.Control.Input.Type;
import Engine.Control.Input.State;
import Engine.Control.Simulation.System;
import std;

namespace
{
    using ::Control::Action::Type::PossessedAction;
    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::Type::ClientSettings;

    using Engine::Control::Input::State::AgentClientStore;
    using Engine::Control::Input::Type::AgentDelivery;

    using Engine::Control::Simulation::System::BotService;
}

export namespace Engine::Control::Input::System
{
    // Runs the client of Artemis for the agents that play an AgentInput (VW.9c.3 C, D363): once per TICK, for each such player, it builds the synthetic
    // frame of the tick, steps the same pure client the possession steps (StepClient) against the unit of the player and takes the result at once.
    // note: Game thread only (the PlayerAction hook calls it once per tick). It never writes game memory: the caller writes the action.
    class AgentClientService
    {
    private:
        using Log = Common::Log::System::Log;

    public:
        AgentClientService(Log& log, BotService& botService, AgentClientStore& agentClientStore) :
            m_Log(log), m_BotService(botService), m_AgentClientStore(agentClientStore) {}
        ~AgentClientService() = default;

        AgentClientService(const AgentClientService&) = delete;
        auto operator=(const AgentClientService&) -> AgentClientService& = delete;

        // Runs one tick of the client for an agent that has an AgentInput.
        // return: What the tick delivers for the player: the override and the one-tick requests, the same as the possession gives. A player without a unit,
        // or a step that did not run, gets the neutral action (the aim of the unit when it has one).
        // param threadContext: Thread context of the engine.
        // param playerIndex: The player of the agent. The caller began its AgentClientStore entry.
        // param delivery: What the PlayerActionStore holds for the player.
        // param settings: The settings of Artemis for agents (ReadAgentClientSettings), read once per tick by the caller. Its tick seconds are the dt of
        // the step: the seconds of one tick of the engine, never the real time (D105).
        // param unit: The unit of the player, nothing when it has none.
        // note: The context has no input user (agents have none): the controls are free, the look rates and the alternate control are the ones of the player
        // of the agent, the dual wield restrictions are 0 and the boost values are the ones of the frame. One WARNING per agent when a step leaves a branch
        // it does not port (Unsupported) or reads a unit fact the context does not know (UnknownRead).
        auto StepTick(std::uintptr_t threadContext, std::size_t playerIndex, const AgentDelivery& delivery, const ClientSettings& settings,
            const std::optional<UnitClientState>& unit) -> PossessedAction;

    private:
        Log& m_Log;
        BotService& m_BotService;
        AgentClientStore& m_AgentClientStore;
    };
}