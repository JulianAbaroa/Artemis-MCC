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
    // reads them. Players are indexed by their player table index. Cleanup is the one call from another thread.
    // Keeps the last window it saw of a possessed player (Suspend), so the end of the possession goes on in repetition (D367, Q112).
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

        // Starts the agent of a player with a new state and zeroed counters, or goes on with the one it has: the one that is active, or the one a
        // possession suspended. Out of range players start nothing.
        // param playerHandle: Handle of the player. An agent of another handle in that index is another agent: it starts new.
        // note: Game thread only.
        auto Begin(std::size_t playerIndex, std::uint32_t playerHandle) -> void;

        // Drops the state of the agent of the player, the one of a suspension too, and marks it inactive (its counters stay readable). Nothing happens
        // for a player with no agent. Game thread only.
        auto End(std::size_t playerIndex) -> void;

        // Ends the agent of a player a possession took (D367): it is inactive, and it only keeps the last delivery it saw with its window over, so the
        // next Begin goes on in repetition (the same held controls and axes, no press, no look) until a new delivery comes. Nothing happens for a
        // player with no active agent. Game thread only.
        auto Suspend(std::size_t playerIndex) -> void;

        // return: The agent of the player, or null when it has none. Game thread only.
        auto Find(std::size_t playerIndex) -> AgentClient*;

        // Counts one tick of the agent. Game thread only.
        // param isDelivered: The step of the client ran (otherwise the tick had a neutral action).
        // param isUnsupported: The step left a branch the client does not port.
        // param isUnknownRead: The step read a unit fact the context does not know.
        auto CountTick(std::size_t playerIndex, bool isDelivered, bool isUnsupported, bool isUnknownRead) -> void;

        // return: The counters of every player and which ones are active. Any thread.
        auto GetStatus() const -> AgentClientStatus;

        // Drops every agent, the suspended ones and the warnings, and zeroes the counters, when a match ends (the panel shows no players of another match).
        // note: Any thread. The counters and the active mask go now. The state of the clients is only touched by the game thread, so it is dropped by the
        // next Begin, End, Suspend or Find (a flag, no lock).
        auto Cleanup() -> void;

    private:
        // Drops the state of every client when Cleanup asked. Game thread only.
        auto ApplyCleanup() -> void;

        // Zeroes the counters and the active mask. Any thread.
        auto ZeroCounters() -> void;

        std::array<AgentClient, k_MaxActionPlayers> m_Clients{};

        // Players (bit per index) a possession suspended: inactive, with only their last delivery kept.
        std::uint32_t m_SuspendedMask{ 0 };
        std::atomic<bool> m_IsCleanupRequested{ false };

        std::array<std::atomic<std::uint64_t>, k_MaxActionPlayers> m_Ticks{};
        std::array<std::atomic<std::uint64_t>, k_MaxActionPlayers> m_Delivered{};
        std::array<std::atomic<std::uint64_t>, k_MaxActionPlayers> m_Unsupported{};
        std::array<std::atomic<std::uint64_t>, k_MaxActionPlayers> m_UnknownRead{};
        std::atomic<std::uint32_t> m_ActiveMask{ 0 };
    };
}