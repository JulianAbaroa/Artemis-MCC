module Tables.Player.State;

namespace Tables::Player::State
{
    auto PlayerTableStore::GetBase() const -> std::uintptr_t
    {
        return m_PlayerTableBase.load();
    }

    auto PlayerTableStore::SetBase(std::uintptr_t pointer) -> void
    {
        m_PlayerTableBase.store(pointer);
    }

    auto PlayerTableStore::AddPlayer(std::uint32_t handle, const AlivePlayer& player) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_PlayerTable.emplace(handle, player);
    }

    auto PlayerTableStore::HasPlayer(std::uint32_t handle) const -> bool
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_PlayerTable.contains(handle);
    }

    auto PlayerTableStore::RemovePlayer(std::uint32_t handle) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_PlayerTable.erase(handle);
    }

    auto PlayerTableStore::Publish() -> void
    {
        std::shared_ptr<const PlayerTable> snapshot{};
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            snapshot = std::make_shared<const PlayerTable>(m_PlayerTable);
        }
        m_PublishedPlayerTable.store(snapshot, std::memory_order_release);
    }

    auto PlayerTableStore::Acquire() const -> std::shared_ptr<const PlayerTable>
    {
        return m_PublishedPlayerTable.load(std::memory_order_acquire);
    }

    auto PlayerTableStore::Cleanup() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_PlayerTable.clear();
        m_PlayerTableBase.store(0);
        m_PublishedPlayerTable.store(nullptr, std::memory_order_release);
    }
}