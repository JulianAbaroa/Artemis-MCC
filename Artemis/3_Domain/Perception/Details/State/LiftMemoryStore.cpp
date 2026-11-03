module Perception.Details.State;
import :LiftMemory;

namespace Perception::Details::State
{
    auto LiftMemoryStore::Get() -> Perception::Details::Type::LiftMemory&
    {
        if (m_IsStale.exchange(false)) m_Memory = {};

        return m_Memory;
    }

    auto LiftMemoryStore::Cleanup() -> void
    {
        m_IsStale.store(true);
    }
}