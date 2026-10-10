module Template.Object.State;

namespace Template::Object::State
{
    auto TemplateStore::GetBase() const -> std::uintptr_t
    {
        return m_ObjectTableBase.load();
    }

    auto TemplateStore::SetBase(std::uintptr_t pointer) -> void
    {
        m_ObjectTableBase.store(pointer);
    }

    auto TemplateStore::Publish(Snapshot snapshot) -> void
    {
        auto published = std::make_shared<const Snapshot>(std::move(snapshot));
        m_Snapshot.store(published, std::memory_order_release);
    }

    auto TemplateStore::Acquire() const -> std::shared_ptr<const Snapshot>
    {
        return m_Snapshot.load(std::memory_order_acquire);
    }

    auto TemplateStore::Cleanup() -> void
    {
        m_ObjectTableBase.store(0);
        m_Snapshot.store(nullptr, std::memory_order_release);
    }
}