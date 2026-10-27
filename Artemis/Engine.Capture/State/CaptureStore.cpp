module Platform.Capture.State;

namespace Platform::Capture::State
{
    auto CaptureStore::GetThreadContext() const -> std::uintptr_t
    {
        return m_ThreadContext.load();
    }

    auto CaptureStore::SetThreadContext(std::uintptr_t pointer) -> void
    {
        m_ThreadContext.store(pointer);
    }

    auto CaptureStore::GetObjectTableBase() const -> std::uintptr_t
    {
        return m_ObjectTableBase.load();
    }

    auto CaptureStore::SetObjectTableBase(std::uintptr_t pointer) -> void
    {
        m_ObjectTableBase.store(pointer);
    }

    auto CaptureStore::GetPlayerTableBase() const -> std::uintptr_t
    {
        return m_PlayerTableBase.load();
    }

    auto CaptureStore::SetPlayerTableBase(std::uintptr_t pointer) -> void
    {
        m_PlayerTableBase.store(pointer);
    }

    auto CaptureStore::GetMappingBase() const -> std::uintptr_t
    {
        return m_MappingBase.load();
    }

    auto CaptureStore::SetMappingBase(std::uintptr_t pointer) -> void
    {
        m_MappingBase.store(pointer);
    }

    auto CaptureStore::GetInteractionTableBase() const -> std::uintptr_t
    {
        return m_InteractionTableBase.load();
    }

    auto CaptureStore::SetInteractionTableBase(std::uintptr_t pointer) -> void
    {
        m_InteractionTableBase.store(pointer);
    }

    auto CaptureStore::Publish(std::shared_ptr<const Snapshot> snapshot) -> void
    {
        m_Snapshot.store(std::move(snapshot), std::memory_order_release);
    }

    auto CaptureStore::Acquire() const -> std::shared_ptr<const Snapshot>
    {
        return m_Snapshot.load(std::memory_order_acquire);
    }

    auto CaptureStore::Cleanup() -> void
    {
        m_ThreadContext.store(0);
        m_ObjectTableBase.store(0);
        m_PlayerTableBase.store(0);
        m_MappingBase.store(0);
        m_InteractionTableBase.store(0);
        m_Snapshot.store(nullptr, std::memory_order_release);
    }
}