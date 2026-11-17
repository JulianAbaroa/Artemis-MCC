module Blam.Runtime.Player.State;

namespace Blam::Runtime::Player::State
{
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
        // Published under the lock, so a Cleanup cannot come between the copy and the store and leave the table of the old map out.
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_PublishedPlayerTable.store(std::make_shared<const PlayerTable>(m_PlayerTable), std::memory_order_release);
    }

    auto PlayerTableStore::Acquire() const -> std::shared_ptr<const PlayerTable>
    {
        return m_PublishedPlayerTable.load(std::memory_order_acquire);
    }

    auto PlayerTableStore::Cleanup() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_PlayerTable.clear();
        m_PublishedPlayerTable.store(nullptr, std::memory_order_release);
    }
}