module UI.PlayerTable.State;

namespace UI::PlayerTable::State
{
	auto PlayerTableUIStore::IsSameSnapshot(
		const std::shared_ptr<const PlayerTable>& table) const -> bool
	{
		return m_Snapshot == table;
	}

	auto PlayerTableUIStore::SetSnapshot(std::shared_ptr<const PlayerTable> table,
		Players players) -> void
	{
		m_Snapshot = std::move(table);
		m_Players = std::move(players);
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
	}
}