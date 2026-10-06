export module UI.ObjectTable.State;

import Export.Tick.Type;
import UI.ObjectTable.Type;
import std;

export namespace UI::ObjectTable::State
{
    // Snapshot of the object table with its alive objects grouped by class.
    class ObjectTableUIStore
    {
    private:
        using ObjectTable = Export::Tick::Type::ObjectTable;
        using AliveObject = Export::Tick::Type::AliveObject;

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

        // Replaces the snapshot and its groups.
        // note: The groups point into the table, so both must be replaced together.
        auto SetSnapshot(std::shared_ptr<const ObjectTable> table, Groups groups) -> void;

        // Returns the alive objects grouped by class.
        auto GetGroups() const -> const Groups&;

        // Returns the number of objects of the snapshot.
        auto GetObjectCount() const -> std::size_t;

        // Drops the snapshot and its groups.
        auto Cleanup() -> void;

    private:
        std::shared_ptr<const ObjectTable> m_Snapshot{};
        Groups m_Groups{};
    };
}