module Relations.PlayerGraph.State;

namespace Relations::PlayerGraph::State
{
    auto PlayerGraphStore::Publish(PlayerTrees trees) -> void
    {
        auto snap = std::make_shared<const PlayerTrees>(std::move(trees));
        m_pPlayerTrees.store(snap, std::memory_order_release);
    }

    auto PlayerGraphStore::Acquire() const -> std::shared_ptr<const PlayerTrees>
    {
        return m_pPlayerTrees.load(std::memory_order_acquire);
    }

    auto PlayerGraphStore::Cleanup() -> void
    {
        m_pPlayerTrees.store(nullptr, std::memory_order_release);
    }
}