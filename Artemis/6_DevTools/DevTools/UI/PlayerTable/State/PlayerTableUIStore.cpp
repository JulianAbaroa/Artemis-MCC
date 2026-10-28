module UI.PlayerTable.State;

import Common.Engine.Type;
import Layout.Player.Type;
import std;

namespace
{
    using Common::Engine::Type::k_HandleIndexMask;
    using Layout::Player::Type::Size::k_Base;
}

namespace UI::PlayerTable::State
{
    auto PlayerTableUIStore::IsSameSnapshot(
        const std::shared_ptr<const PlayerTable>& table) const -> bool
    {
        return m_Snapshot == table;
    }

    auto PlayerTableUIStore::SetSnapshot(std::shared_ptr<const PlayerTable> table,
        Players players, std::uintptr_t arrayAddress) -> void
    {
        m_Snapshot = std::move(table);
        m_Players = std::move(players);
        m_ArrayAddress = arrayAddress;
    }

    auto PlayerTableUIStore::GetAddress(std::uint32_t handle) const -> std::uintptr_t
    {
        if (m_ArrayAddress == 0) return 0;

        return m_ArrayAddress + (handle & k_HandleIndexMask) * k_Base;
    }

    auto PlayerTableUIStore::GetPlayers() const -> const Players&
    {
        return m_Players;
    }

    auto PlayerTableUIStore::GetPlayerCount() const -> std::size_t
    {
        return m_Snapshot ? m_Snapshot->size() : 0;
    }

    auto PlayerTableUIStore::Cleanup() -> void
    {
        m_Players.clear();
        m_Snapshot.reset();
        m_ArrayAddress = 0;
    }
}