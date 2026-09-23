module UI.ObjectTable.State;

namespace UI::ObjectTable::State
{
	auto ObjectTableUIStore::GetSnapshot() const -> const ObjectTable*
	{
		return m_Snapshot.get();
	}

	auto ObjectTableUIStore::IsSameSnapshot(
		const std::shared_ptr<const ObjectTable>& table) const -> bool
	{
		return m_Snapshot == table;
	}

	auto ObjectTableUIStore::SetSnapshot(std::shared_ptr<const ObjectTable> table,
		Groups groups) -> void
	{
		m_Snapshot = std::move(table);
		m_Groups = std::move(groups);
	}

	auto ObjectTableUIStore::GetGroups() const -> const Groups&
	{
		return m_Groups;
	}

	auto ObjectTableUIStore::GetObjectCount() const -> std::size_t
	{
		return m_Snapshot ? m_Snapshot->size() : 0;
	}

	auto ObjectTableUIStore::Cleanup() -> void
	{
		m_Groups.clear();
		m_Snapshot.reset();
	}
}