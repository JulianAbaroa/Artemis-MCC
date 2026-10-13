module Datum.Object.State;

namespace Datum::Object::State
{
    auto DatumStore::GetBase() const -> std::uintptr_t
    {
        return m_ObjectTableBase.load();
    }

    auto DatumStore::SetBase(std::uintptr_t pointer) -> void
    {
        m_ObjectTableBase.store(pointer);
    }

    auto DatumStore::Publish(Snapshot snapshot) -> void
    {
        auto published = std::make_shared<const Snapshot>(std::move(snapshot));
        m_Snapshot.store(published, std::memory_order_release);
    }

    auto DatumStore::Acquire() const -> std::shared_ptr<const Snapshot>
    {
        return m_Snapshot.load(std::memory_order_acquire);
    }

    auto DatumStore::Cleanup() -> void
    {
        m_ObjectTableBase.store(0);
        m_Snapshot.store(nullptr, std::memory_order_release);
    }
}