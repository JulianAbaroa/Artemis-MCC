module Engine.Base.Pov.State;
import :Status;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

namespace Engine::Base::Pov::State
{
    auto PovStatusStore::SetHandles(std::uint32_t requested, std::uint32_t engine, std::uint32_t local, std::uint32_t assigned) -> void
    {
        m_Requested.store(requested, std::memory_order_relaxed);
        m_Engine.store(engine, std::memory_order_relaxed);
        m_Local.store(local, std::memory_order_relaxed);
        m_Assigned.store(assigned, std::memory_order_relaxed);
    }

    auto PovStatusStore::CountCall(bool isReassert) -> void
    {
        m_Calls.fetch_add(1, std::memory_order_relaxed);
        if (isReassert) m_Reasserts.fetch_add(1, std::memory_order_relaxed);
    }

    auto PovStatusStore::CountFault() -> void
    {
        m_Faults.fetch_add(1, std::memory_order_relaxed);
    }

    auto PovStatusStore::ResetCounts() -> void
    {
        m_Calls.store(0, std::memory_order_relaxed);
        m_Reasserts.store(0, std::memory_order_relaxed);
    }

    auto PovStatusStore::Reset() -> void
    {
        this->SetHandles(k_NoHandle, k_NoHandle, k_NoHandle, k_NoHandle);
        m_Calls.store(0, std::memory_order_relaxed);
        m_Reasserts.store(0, std::memory_order_relaxed);
        m_Faults.store(0, std::memory_order_relaxed);
        m_IsInstalled.store(false, std::memory_order_relaxed);
    }

    auto PovStatusStore::SetInstalled(bool isInstalled) -> void
    {
        m_IsInstalled.store(isInstalled, std::memory_order_relaxed);
    }

    auto PovStatusStore::Read() const -> PovStatus
    {
        return PovStatus{
            .RequestedHandle = m_Requested.load(std::memory_order_relaxed),
            .EngineHandle = m_Engine.load(std::memory_order_relaxed),
            .LocalHandle = m_Local.load(std::memory_order_relaxed),
            .AssignedHandle = m_Assigned.load(std::memory_order_relaxed),
            .Calls = m_Calls.load(std::memory_order_relaxed),
            .Reasserts = m_Reasserts.load(std::memory_order_relaxed),
            .Faults = m_Faults.load(std::memory_order_relaxed),
            .IsInstalled = m_IsInstalled.load(std::memory_order_relaxed) };
    }
}