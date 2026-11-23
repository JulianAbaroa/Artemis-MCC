export module Engine.Control.Input.State:AgentClient;

import Engine.Control.Input.Type;
import Blam.Layout.Action.Type;
import std;

namespace
{
    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
}

export namespace Engine::Control::Input::State
{
    // What the client of Artemis keeps for each player whose AgentInput it plays, and the counters of the panel (VW.9c.3 C).
    // note: The state is touched by the game thread only (the PlayerAction hook, once per tick). The counters and the active mask are atomic: the UI
    // reads them. Players are indexed by their player table index.
    class AgentClientStore
    {
    private:
        using AgentClient = Engine::Control::Input::Type::AgentClient;
        using AgentCounters = Engine::Control::Input::Type::AgentCounters;
        using AgentClientStatus = Engine::Control::Input::Type::AgentClientStatus;

    public:
        AgentClientStore() = default;
        ~AgentClientStore() = default;

        AgentClientStore(const AgentClientStore&) = delete;
        auto operator=(const AgentClientStore&) -> AgentClientStore& = delete;

        // Starts the agent of a player with a new state and zeroed counters, or goes on with the one it has.
        // return: True when it started a new agent (the state is new), false when it goes on. Out of range players start nothing.
        // note: Game thread only.
        auto Begin(std::size_t playerIndex) -> bool;

        // Drops the state of the agent of the player and marks it inactive (its counters stay readable). Game thread only.
        auto End(std::size_t playerIndex) -> void;

        // return: The agent of the player, or null when it has none. Game thread only.
        auto Find(std::size_t playerIndex) -> AgentClient*;

        // Counts one tick of the agent. Game thread only.
        // param isDelivered: The step of the client ran (otherwise the tick had a neutral action).
        // param isUnsupported: The step left a branch the client does not port.
        // param isUnknownRead: The step read a unit fact the context does not know.
        auto CountTick(std::size_t playerIndex, bool isDelivered, bool isUnsupported, bool isUnknownRead) -> void;

        // return: The counters of every player and which ones are active. Any thread.
        auto GetStatus() const -> AgentClientStatus;

    private:
        std::array<AgentClient, k_MaxActionPlayers> m_Clients{};

        std::array<std::atomic<std::uint64_t>, k_MaxActionPlayers> m_Ticks{};
        std::array<std::atomic<std::uint64_t>, k_MaxActionPlayers> m_Delivered{};
        std::array<std::atomic<std::uint64_t>, k_MaxActionPlayers> m_Unsupported{};
        std::array<std::atomic<std::uint64_t>, k_MaxActionPlayers> m_UnknownRead{};
        std::atomic<std::uint32_t> m_ActiveMask{ 0 };
    };
}