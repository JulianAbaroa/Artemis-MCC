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
    using ::Control::Action::Type::ClientAction;
    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::Type::ClientSettings;

    using Engine::Control::Input::State::AgentClientStore;
    using Engine::Control::Input::State::PlayerActionStore;
    using Engine::Control::Input::Type::AgentClient;
    using Engine::Control::Input::Type::AgentDelivery;

    using Engine::Control::Simulation::System::BotService;
}

export namespace Engine::Control::Input::System
{
    // Runs the client of Artemis for the agents that play an AgentInput (VW.9c.3 C, D363): once per TICK, for each such player, it builds the synthetic
    // frame of the tick, steps the same pure client the possession steps (StepClient) against the unit of the player and takes the result at once.
    // It also decides who plays (VW.9c.3 D): it begins, suspends (possession, D367) or ends the client of each player and counts every tick, so the hook
    // only wires it.
    // note: Game thread only (the PlayerAction hook calls it once per tick). It never writes game memory: the caller writes the action.
    class AgentClientService
    {
    private:
        using Log = Common::Log::System::Log;

    public:
        AgentClientService(Log& log, BotService& botService, AgentClientStore& agentClientStore, PlayerActionStore& playerActionStore) :
            m_Log(log), m_BotService(botService), m_AgentClientStore(agentClientStore), m_PlayerActionStore(playerActionStore) {}
        ~AgentClientService() = default;

        AgentClientService(const AgentClientService&) = delete;
        auto operator=(const AgentClientService&) -> AgentClientService& = delete;

        // Plays one tick of a player, with or without an AgentInput.
        // return: What the tick delivers for the player: the override and the one-tick requests, the same as the possession gives. A player without a unit,
        // without settings or a step that did not run gets the neutral action (the aim of the unit when it has one). Nothing when the player is no agent
        // this tick: it has no AgentInput, it is possessed (Possession wins, D351, its client is suspended and its AgentInput waits) or the AgentInput is
        // of another player (Stale: the handle in the index changed, the delivery is dropped).
        // param threadContext: Thread context of the engine.
        // param playerIndex: The player.
        // param possessed: The possessed player, nothing when none.
        // param delivery: What the PlayerActionStore holds for the player, nothing when it has none.
        // param settings: The settings of Artemis for agents (ReadAgentClientSettings), read once per tick by the caller, nothing when they are not known.
        // Its tick seconds are the dt of the step: the seconds of one tick of the engine, never the real time (D105).
        // note: The window of the agent advances on every tick it plays, with or without settings or unit. The context has no input user (agents have
        // none): the controls are free, the look rates and the alternate control are the ones of the player of the agent, the dual wield restrictions are 0
        // and the boost values are the ones of the frame. One WARNING per agent when a step leaves a branch it does not port (Unsupported) or reads a unit
        // fact the context does not know (UnknownRead). A step that did not run leaves the synthesizer as it was (UndoAgentFrame).
        auto PlayTick(std::uintptr_t threadContext, std::size_t playerIndex, std::optional<std::size_t> possessed, const std::optional<AgentDelivery>& delivery,
            const std::optional<ClientSettings>& settings) -> std::optional<ClientAction>;

        // Suspends the client of every active player (the hook does not run the actions this tick): each keeps its last window, so it never plays again.
        auto SuspendAll() -> void;

    private:
        // Steps the client of an agent that plays this tick with its unit.
        auto StepUnit(std::uintptr_t threadContext, std::size_t playerIndex, std::uint32_t playerHandle, AgentClient& agent, const AgentDelivery& delivery,
            std::uint32_t tickInWindow, const ClientSettings& settings, const UnitClientState& unit) -> ClientAction;

        Log& m_Log;
        BotService& m_BotService;
        AgentClientStore& m_AgentClientStore;
        PlayerActionStore& m_PlayerActionStore;
    };
}