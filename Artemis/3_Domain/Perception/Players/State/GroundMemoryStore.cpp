module Perception.Players.State;
import :GroundMemory;

namespace Perception::Players::State
{
    auto GroundMemoryStore::Get() -> Perception::Players::Type::GroundMemory&
    {
        if (m_IsStale.exchange(false)) m_Memory = {};

        return m_Memory;
    }

    auto GroundMemoryStore::Cleanup() -> void
    {
        m_IsStale.store(true);
    }
}