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
    auto AgentClientStore::Begin(std::size_t playerIndex, std::uint32_t playerHandle) -> void
    {
        if (playerIndex >= k_MaxActionPlayers) return;

        this->ApplyCleanup();

        const std::uint32_t bit{ 1u << playerIndex };
        AgentClient& client{ m_Clients[playerIndex] };
        const bool isActive{ (m_ActiveMask.load() & bit) != 0 };
        const bool isSuspended{ (m_SuspendedMask & bit) != 0 };
        if ((isActive || isSuspended) && client.Client.Handle == playerHandle)
        {
            m_SuspendedMask &= ~bit;
            m_ActiveMask.fetch_or(bit);
            return;
        }

        client = AgentClient{ .Client = { .Player = playerIndex, .Handle = playerHandle } };
        m_SuspendedMask &= ~bit;
        m_Ticks[playerIndex].store(0);
        m_Delivered[playerIndex].store(0);
        m_Unsupported[playerIndex].store(0);
        m_UnknownRead[playerIndex].store(0);
        m_ActiveMask.fetch_or(bit);
    }

    auto AgentClientStore::End(std::size_t playerIndex) -> void
    {
        if (playerIndex >= k_MaxActionPlayers) return;

        this->ApplyCleanup();

        const std::uint32_t bit{ 1u << playerIndex };
        if (((m_ActiveMask.load() | m_SuspendedMask) & bit) == 0) return;

        m_ActiveMask.fetch_and(~bit);
        m_SuspendedMask &= ~bit;
        m_Clients[playerIndex] = AgentClient{};
    }

    auto AgentClientStore::Suspend(std::size_t playerIndex) -> void
    {
        if (playerIndex >= k_MaxActionPlayers) return;

        this->ApplyCleanup();

        const std::uint32_t bit{ 1u << playerIndex };
        if ((m_ActiveMask.load() & bit) == 0) return;

        AgentClient& client{ m_Clients[playerIndex] };
        client = AgentClient{ .Client = { .Player = playerIndex, .Handle = client.Client.Handle },
            .Input = client.Input, .HasSequence = client.HasSequence, .Sequence = client.Sequence, .TickInWindow = (std::numeric_limits<std::uint32_t>::max)(),
            .IsWarned = client.IsWarned, .ResumedUnit = client.Client.Unit };
        m_ActiveMask.fetch_and(~bit);
        m_SuspendedMask |= bit;
    }

    auto AgentClientStore::Find(std::size_t playerIndex) -> AgentClient*
    {
        this->ApplyCleanup();

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

    auto AgentClientStore::Cleanup() -> void
    {
        this->ZeroCounters();
        m_IsCleanupRequested.store(true);
    }

    auto AgentClientStore::ApplyCleanup() -> void
    {
        if (!m_IsCleanupRequested.exchange(false)) return;

        // note: Zeroed again here: a tick of the game thread that ran while Cleanup did may have counted or begun an agent of the old match.
        this->ZeroCounters();
        m_Clients.fill(AgentClient{});
        m_SuspendedMask = 0;
    }

    auto AgentClientStore::ZeroCounters() -> void
    {
        m_ActiveMask.store(0);
        for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player)
        {
            m_Ticks[player].store(0);
            m_Delivered[player].store(0);
            m_Unsupported[player].store(0);
            m_UnknownRead[player].store(0);
        }
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