export module UI.ObjectTable.State;

import Layout.Snapshot.Type;
import Tables.Object.Type;
import Export.Tick.Type;
import UI.ObjectTable.Type;
import std;

export namespace UI::ObjectTable::State
{
    // Snapshot of the object table with its alive objects grouped by class.
    class ObjectTableUIStore
    {
    private:
        using Snapshot = Layout::Snapshot::Type::Snapshot;

        using ObjectTable = Tables::Object::Type::Alive::ObjectTable;
        using AliveObject = Tables::Object::Type::Alive::AliveObject;

        using Groups = UI::ObjectTable::Type::Groups;

    public:
        ObjectTableUIStore() = default;
        ~ObjectTableUIStore() = default;

        ObjectTableUIStore(const ObjectTableUIStore&) = delete;
        auto operator=(const ObjectTableUIStore&) -> ObjectTableUIStore& = delete;

        // Returns the snapshot or nullptr when there is none.
        auto GetSnapshot() const -> const ObjectTable*;

        // Checks whether the table is the stored snapshot.
        auto IsSameSnapshot(const std::shared_ptr<const ObjectTable>& table) const -> bool;

        // Replaces the snapshot, its groups and the published snapshot the addresses are read from.
        // note: The groups point into the table, so both must be replaced together.
        auto SetSnapshot(std::shared_ptr<const ObjectTable> table, Groups groups, std::shared_ptr<const Snapshot> published) -> void;

        // return: The published snapshot taken with the table, or nullptr when there is none.
        auto GetPublished() const -> const Snapshot*;

        // Returns the alive objects grouped by class.
        auto GetGroups() const -> const Groups&;

        // Returns the number of objects of the snapshot.
        auto GetObjectCount() const -> std::size_t;

        // Drops the snapshot and its groups.
        auto Cleanup() -> void;

    private:
        std::shared_ptr<const ObjectTable> m_Snapshot{};
        std::shared_ptr<const Snapshot> m_Published{};
        Groups m_Groups{};
    };
}