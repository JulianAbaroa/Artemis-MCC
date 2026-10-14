module Environment.Entities.State;
import :CarriedHandles;

namespace Environment::Entities::State
{
    auto CarriedHandlesStore::Contains(std::uint32_t handle) -> bool
    {
        this->DropIfStale();

        return m_Handles.contains(handle);
    }

    auto CarriedHandlesStore::Add(std::uint32_t handle) -> void
    {
        this->DropIfStale();

        m_Handles.insert(handle);
    }

    auto CarriedHandlesStore::Cleanup() -> void
    {
        m_IsStale.store(true);
    }

    auto CarriedHandlesStore::DropIfStale() -> void
    {
        if (m_IsStale.exchange(false)) m_Handles.clear();
    }
}