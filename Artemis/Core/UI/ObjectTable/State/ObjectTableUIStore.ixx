export module UI.ObjectTable.State;

import Export.Tick.Type;
import std;

export namespace UI::ObjectTable::State
{
	class ObjectTableUIStore
	{
	private:
		using ObjectTable = Export::Tick::Type::ObjectTable;
		using AliveObject = Export::Tick::Type::AliveObject;

	public:
		using Group = std::vector<const AliveObject*>;
		using Groups = std::map<std::string, Group>;

		ObjectTableUIStore() = default;
		~ObjectTableUIStore() = default;

		ObjectTableUIStore(const ObjectTableUIStore&) = delete;
		ObjectTableUIStore& operator=(const ObjectTableUIStore&) = delete;

		auto GetSnapshot() const -> const ObjectTable*;
		auto IsSameSnapshot(const std::shared_ptr<const ObjectTable>& table) const -> bool;
		auto SetSnapshot(std::shared_ptr<const ObjectTable> table, Groups groups) -> void;

		auto GetGroups() const -> const Groups&;
		auto GetObjectCount() const -> std::size_t;

		auto Cleanup() -> void;

	private:
		std::shared_ptr<const ObjectTable> m_Snapshot{};
		Groups m_Groups{};
	};
}