export module Blam.Runtime.Object.State;

export import :BoneMatrices;
export import :DamageSections;

import Blam.Runtime.Object.Type;
import std;

export namespace Blam::Runtime::Object::State
{
    class ObjectTableStore
    {
    private:
        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using ObjectTable = Blam::Runtime::Object::Type::Alive::ObjectTable;
        using DeadObject = Blam::Runtime::Object::Type::Slot::DeadObject;
        using ObjectSlots = Blam::Runtime::Object::Type::Slot::ObjectSlots;
        using PooledTable = Blam::Runtime::Object::Type::Slot::PooledTable;

    public:
        ObjectTableStore() = default;
        ~ObjectTableStore() = default;

        // Each of these takes the lock of the store.
        auto AddObject(std::uint32_t handle, const AliveObject& object) -> void;
        auto RemoveObject(std::uint32_t handle) -> std::optional<AliveObject>;

        // Runs the editor holding the lock of the store once, so it can use the row functions below without a lock per object.
        // note: AI thread only, and the editor must not call the functions above (the lock is not recursive).
        auto Edit(const std::function<void()>& editor) -> void;

        // Row functions: only inside Edit. They work by slot, with no search.
        // return: The row of the handle, or null if the slot holds no row or another handle.
        auto FindRow(std::uint32_t handle) -> AliveObject*;

        // return: The handle of the row that holds the slot of this handle, or none.
        auto SlotHandle(std::uint32_t handle) const -> std::uint32_t;

        // Adds the row of the handle and counts it as changed. A row of the same handle that exists stays as it is, and a row of another
        // handle in the same slot is removed (the engine reused the slot).
        // return: The row.
        auto AddRow(std::uint32_t handle, const AliveObject& object) -> AliveObject&;

        auto RemoveRow(std::uint32_t handle) -> void;

        // Counts the row as changed for the next Publish: the caller wrote in it.
        auto MarkChanged(std::uint32_t handle) -> void;

        // Publishes the live table. The table is written over a table of a small pool that nobody holds, so the steady state allocates no
        // table and frees none, and only the rows that changed since that table was published are copied: the rows of the objects that left
        // are erased from it, and the rows that were added or changed are written. A table that is new, or that the store cannot bring up
        // to date, is a copy of the whole live table. When every pooled table is taken (a reader keeps it), a new one is made and pooled up
        // to k_PoolCapacity, and a table above that is made and not kept. The result is the same as a copy of the whole table.
        // note: AI thread only. use_count is a loose measure that is enough here: only this thread publishes, and a reader that dropped a
        // table cannot take it again, because Acquire gives the published table and a pooled table that is not published is never it.
        auto Publish() -> void;
        auto Acquire() const -> std::shared_ptr<const ObjectTable>;

        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const ObjectTable>> m_PublishedTable{};

        // Tables Publish may write over when only the pool holds them.
        static constexpr std::size_t k_PoolCapacity{ 4 };
        // Objects that left and the pooled tables did not hear of yet. Past this the log is dropped and every pooled table is copied whole.
        static constexpr std::size_t k_DeadCapacity{ 4096 };

        ObjectTable m_ObjectTable{};
        ObjectSlots m_Slots{};
        std::vector<DeadObject> m_Dead{};
        std::vector<PooledTable> m_Pool{};
        // Tick of the table being built: the changes of this tick belong to the next Publish. Starts at 1, so a pooled table at 0 is stale.
        std::uint64_t m_Tick{ 1 };
        // Whether some row has no slot (a handle index of k_MaxObjects or more), which no row function can follow: Publish copies whole.
        bool m_IsUntracked{};
        mutable std::mutex m_Mutex{};

        auto AddRowLocked(std::uint32_t handle, const AliveObject& object) -> AliveObject&;
        auto RemoveRowLocked(std::uint32_t handle) -> std::optional<AliveObject>;
    };
}