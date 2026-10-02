module Environment.Aim.State;

namespace Environment::Aim::State
{
    auto AimStore::Publish(Aims aims) -> void
    {
        auto snapshot = std::make_shared<const Aims>(std::move(aims));

        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Aims = std::move(snapshot);
    }

    auto AimStore::Acquire() const -> std::shared_ptr<const Aims>
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Aims;
    }

    auto AimStore::Clear() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Aims.reset();
    }
}