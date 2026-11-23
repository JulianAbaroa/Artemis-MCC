module Engine.Control.Input.State;
import :AgentClient;

import Blam.Layout.Action.Type;
import std;

namespace
{
    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
}

namespace Engine::Control::Input::State
{
    auto AgentClientStore::Begin(std::size_t playerIndex) -> bool
    {
        if (playerIndex >= k_MaxActionPlayers) return false;

        const std::uint32_t bit{ 1u << playerIndex };
        if ((m_ActiveMask.load() & bit) != 0) return false;

        m_Clients[playerIndex] = AgentClient{ .Client = { .Player = playerIndex } };
        m_Ticks[playerIndex].store(0);
        m_Delivered[playerIndex].store(0);
        m_Unsupported[playerIndex].store(0);
        m_UnknownRead[playerIndex].store(0);
        m_ActiveMask.fetch_or(bit);
        return true;
    }

    auto AgentClientStore::End(std::size_t playerIndex) -> void
    {
        if (playerIndex >= k_MaxActionPlayers) return;

        m_ActiveMask.fetch_and(~(1u << playerIndex));
        m_Clients[playerIndex] = AgentClient{};
    }

    auto AgentClientStore::Find(std::size_t playerIndex) -> AgentClient*
    {
        if (playerIndex >= k_MaxActionPlayers || (m_ActiveMask.load() & (1u << playerIndex)) == 0) return nullptr;

        return &m_Clients[playerIndex];
    }

    auto AgentClientStore::CountTick(std::size_t playerIndex, bool isDelivered, bool isUnsupported, bool isUnknownRead) -> void
    {
        if (playerIndex >= k_MaxActionPlayers) return;

        m_Ticks[playerIndex].fetch_add(1);
        if (isDelivered) m_Delivered[playerIndex].fetch_add(1);
        if (isUnsupported) m_Unsupported[playerIndex].fetch_add(1);
        if (isUnknownRead) m_UnknownRead[playerIndex].fetch_add(1);
    }

    auto AgentClientStore::GetStatus() const -> AgentClientStatus
    {
        AgentClientStatus status{};
        for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player)
        {
            status.Players[player] = AgentCounters{ .Ticks = m_Ticks[player].load(), .Delivered = m_Delivered[player].load(),
                .Unsupported = m_Unsupported[player].load(), .UnknownRead = m_UnknownRead[player].load() };
        }

        status.ActiveMask = m_ActiveMask.load();
        return status;
    }
}