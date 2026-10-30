module Perception.Events.State;
import :Memory;

namespace Perception::Events::State
{
    auto EventsMemoryStore::Get() -> Perception::Events::Type::EventsMemory&
    {
        if (m_IsStale.exchange(false)) m_Memory = {};

        return m_Memory;
    }

    auto EventsMemoryStore::Cleanup() -> void
    {
        m_IsStale.store(true);
    }
}