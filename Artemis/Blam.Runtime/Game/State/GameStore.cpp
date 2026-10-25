module Tables.Game.State;

namespace
{
    using Common::State::Type::Snapshot;
    using Tables::Game::Type::Alive::AliveGame;
}

namespace Tables::Game::State
{
    auto GameStore::GetEventCursor() const -> std::uint32_t
    {
        return m_EventCursor.load();
    }

    auto GameStore::SetEventCursor(std::uint32_t sequence) -> void
    {
        m_EventCursor.store(sequence);
    }

    auto GameStore::GetDroppedEvents() const -> std::uint64_t
    {
        return m_DroppedEvents.load();
    }

    auto GameStore::AddDroppedEvents(std::uint64_t count) -> void
    {
        m_DroppedEvents.fetch_add(count);
    }

    auto GameStore::Cleanup() -> void
    {
        this->Snapshot<AliveGame>::Cleanup();
        m_EventCursor.store(0);
        m_DroppedEvents.store(0);
    }
}