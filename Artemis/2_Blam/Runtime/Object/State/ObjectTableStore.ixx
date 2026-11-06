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

    public:
        ObjectTableStore() = default;
        ~ObjectTableStore() = default;

        auto AddObject(std::uint32_t handle, const AliveObject& object) -> void;
        auto HasObject(std::uint32_t handle) const -> bool;
        auto RemoveObject(std::uint32_t handle) -> std::optional<AliveObject>;

        auto UpdateObjects(std::function<void(std::uint32_t, AliveObject&)> processor) -> void;

        // Publishes a copy of the live table. The copy is written over a table of a small pool that nobody holds, so the steady state
        // allocates no table and frees none. When every pooled table is taken (a reader keeps it), a new one is made and pooled up to
        // k_PoolCapacity, and a table above that is made and not kept.
        // note: AI thread only. use_count is a loose measure that is enough here: only this thread publishes, and a reader that dropped a
        // table cannot take it again, because Acquire gives the published table and a pooled table that is not published is never it.
        auto Publish() -> void;
        auto Acquire() const -> std::shared_ptr<const ObjectTable>;

        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const ObjectTable>> m_PublishedTable{};

        // Tables Publish may write over when only the pool holds them.
        static constexpr std::size_t k_PoolCapacity{ 4 };

        ObjectTable m_ObjectTable{};
        std::vector<std::shared_ptr<ObjectTable>> m_Pool{};
        mutable std::mutex m_Mutex{};
    };
}