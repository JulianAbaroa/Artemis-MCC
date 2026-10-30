module Perception.Details.State;
import :ChargeMemory;

namespace Perception::Details::State
{
    auto WeaponChargeMemoryStore::Get() -> Perception::Details::Type::WeaponChargeMemory&
    {
        if (m_IsStale.exchange(false)) m_Memory = {};

        return m_Memory;
    }

    auto WeaponChargeMemoryStore::Cleanup() -> void
    {
        m_IsStale.store(true);
    }
}