module Blam.Runtime.Object.State;

import Common.Engine.Type;

namespace
{
    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Engine::Type::k_MaxObjects;
    using Common::Engine::Type::k_NoHandle;
}

namespace Blam::Runtime::Object::State
{
    auto ObjectTableStore::AddObject(std::uint32_t handle, const AliveObject& object) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        this->AddRowLocked(handle, object);
    }

    auto ObjectTableStore::RemoveObject(std::uint32_t handle) -> std::optional<AliveObject>
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return this->RemoveRowLocked(handle);
    }

    auto ObjectTableStore::Edit(const std::function<void()>& editor) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        editor();
    }

    auto ObjectTableStore::FindRow(std::uint32_t handle) -> AliveObject*
    {
        const std::uint32_t slot{ handle & k_HandleIndexMask };
        if (slot >= k_MaxObjects)
        {
            auto it = m_ObjectTable.find(handle);
            return it != m_ObjectTable.end() ? &it->second : nullptr;
        }

        return m_Slots[slot].Handle == handle ? m_Slots[slot].Object : nullptr;
    }

    auto ObjectTableStore::SlotHandle(std::uint32_t handle) const -> std::uint32_t
    {
        const std::uint32_t slot{ handle & k_HandleIndexMask };
        return slot < k_MaxObjects ? m_Slots[slot].Handle : k_NoHandle;
    }

    auto ObjectTableStore::AddRow(std::uint32_t handle, const AliveObject& object) -> AliveObject&
    {
        return this->AddRowLocked(handle, object);
    }

    auto ObjectTableStore::RemoveRow(std::uint32_t handle) -> void
    {
        this->RemoveRowLocked(handle);
    }

    auto ObjectTableStore::MarkChanged(std::uint32_t handle) -> void
    {
        const std::uint32_t slot{ handle & k_HandleIndexMask };
        if (slot < k_MaxObjects && m_Slots[slot].Handle == handle) m_Slots[slot].ChangedTick = m_Tick;
    }

    auto ObjectTableStore::AddRowLocked(std::uint32_t handle, const AliveObject& object) -> AliveObject&
    {
        const std::uint32_t slot{ handle & k_HandleIndexMask };
        if (slot < k_MaxObjects && m_Slots[slot].Handle == handle) return *m_Slots[slot].Object;

        // The old object of the slot left: it gets its death record.
        if (slot < k_MaxObjects && m_Slots[slot].Handle != k_NoHandle) this->RemoveRowLocked(m_Slots[slot].Handle);

        auto [it, isAdded] = m_ObjectTable.emplace(handle, object);
        if (slot >= k_MaxObjects)
        {
            if (isAdded) m_IsUntracked = true;
            return it->second;
        }

        m_Slots[slot] = { .Handle = handle, .Object = &it->second, .ChangedTick = m_Tick };
        return it->second;
    }

    auto ObjectTableStore::RemoveRowLocked(std::uint32_t handle) -> std::optional<AliveObject>
    {
        auto it = m_ObjectTable.find(handle);
        if (it == m_ObjectTable.end()) return std::nullopt;

        AliveObject removedCopy = std::move(it->second);
        m_ObjectTable.erase(it);

        const std::uint32_t slot{ handle & k_HandleIndexMask };
        if (slot < k_MaxObjects && m_Slots[slot].Handle == handle) m_Slots[slot] = {};

        m_Dead.push_back({ .Handle = handle, .Tick = m_Tick });
        return removedCopy;
    }

    auto ObjectTableStore::Publish() -> void
    {
        std::shared_ptr<ObjectTable> target;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            // The published table has a second reference (the published pointer), so it is never chosen.
            PooledTable* pooled{ nullptr };
            for (PooledTable& candidate : m_Pool)
            {
                if (candidate.Table.use_count() != 1) continue;

                pooled = &candidate;
                break;
            }

            if (pooled != nullptr && pooled->Tick != 0)
            {
                target = pooled->Table;
                const std::uint64_t since{ pooled->Tick };

                for (const DeadObject& dead : m_Dead)
                {
                    if (dead.Tick > since) target->erase(dead.Handle);
                }

                for (const auto& slot : m_Slots)
                {
                    if (slot.Object != nullptr && slot.ChangedTick > since) target->insert_or_assign(slot.Handle, *slot.Object);
                }
            }
            else if (pooled != nullptr)
            {
                target = pooled->Table;
                *target = m_ObjectTable;
            }
            else
            {
                target = std::make_shared<ObjectTable>(m_ObjectTable);
                if (m_Pool.size() < k_PoolCapacity) pooled = &m_Pool.emplace_back(PooledTable{ .Table = target });
            }

            const std::uint64_t publishedTick{ m_IsUntracked ? 0 : m_Tick };
            if (pooled != nullptr) pooled->Tick = publishedTick;

            // The log keeps what the oldest pooled table does not know yet. A table that is held, or that waits to be chosen, has an old tick.
            if (m_Dead.size() > k_DeadCapacity)
            {
                m_Dead.clear();
                for (PooledTable& entry : m_Pool) entry.Tick = 0;
            }
            else
            {
                std::uint64_t oldest{ std::numeric_limits<std::uint64_t>::max() };
                for (const PooledTable& entry : m_Pool)
                {
                    if (entry.Tick != 0) oldest = (std::min)(oldest, entry.Tick);
                }

                std::erase_if(m_Dead, [oldest](const DeadObject& dead) { return dead.Tick <= oldest; });
            }

            ++m_Tick;
        }
        m_PublishedTable.store(target, std::memory_order_release);
    }

    auto ObjectTableStore::Acquire() const -> std::shared_ptr<const ObjectTable>
    {
        return m_PublishedTable.load(std::memory_order_acquire);
    }

    auto ObjectTableStore::Cleanup() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_ObjectTable.clear();
        m_Slots = {};
        m_Dead.clear();
        m_IsUntracked = false;
        m_PublishedTable.store(nullptr, std::memory_order_release);
        m_Pool.clear();
    }
}