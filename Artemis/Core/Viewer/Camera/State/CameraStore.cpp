module Viewer.Camera.State;

namespace Viewer::Camera::State
{
    auto CameraStore::IsActive() const -> bool
    {
        return m_IsActive.load();
    }

    auto CameraStore::SetActive(bool value) -> void
    {
        m_IsActive.store(value);
    }

    auto CameraStore::GetMode() const -> Mode
    {
        return m_Mode.load(std::memory_order_relaxed);
    }

    auto CameraStore::SetMode(Mode mode) -> void
    {
        if (m_Mode.exchange(mode, std::memory_order_relaxed) == mode) return;

        this->ResetKeys();

        float dx = 0.0f, dy = 0.0f;
        this->ConsumeMouseDelta(dx, dy);
    }

    auto CameraStore::ToggleMode() -> void
    {
        this->SetMode(this->GetMode() == Mode::Free ? Mode::Game : Mode::Free);
    }

    auto CameraStore::IsZoomActive() const -> bool
    {
        return m_IsZoomActive.load(std::memory_order_relaxed);
    }

    auto CameraStore::SetZoomActive(bool value) -> void
    {
        m_IsZoomActive.store(value, std::memory_order_relaxed);
    }

    auto CameraStore::ToggleZoom() -> void
    {
        bool expected = m_IsZoomActive.load(std::memory_order_relaxed);

        while (!m_IsZoomActive.compare_exchange_weak(expected, !expected, std::memory_order_relaxed))
        {
        }
    }

    auto CameraStore::GetFollow() const -> Follow
    {
        return m_Follow.load(std::memory_order_relaxed);
    }

    auto CameraStore::SetFollow(Follow follow) -> void
    {
        m_Follow.store(follow, std::memory_order_relaxed);
    }

    auto CameraStore::CycleFollow() -> void
    {
        switch (this->GetFollow())
        {
        case Follow::Off:         this->SetFollow(Follow::FirstPerson); break;
        case Follow::FirstPerson: this->SetFollow(Follow::Orbit); break;
        default:                  this->SetFollow(Follow::Off); break;
        }
    }

    auto CameraStore::GetOrbitDistance() const -> float
    {
        return m_OrbitDistance.load(std::memory_order_relaxed);
    }

    auto CameraStore::SetOrbitDistance(float value) -> void
    {
        m_OrbitDistance.store(std::clamp(value, k_MinOrbitDistance, k_MaxOrbitDistance),
            std::memory_order_relaxed);
    }

    auto CameraStore::IsKeyDown(Key key) const -> bool
    {
        return m_Keys[static_cast<std::size_t>(key)].load(std::memory_order_relaxed);
    }

    auto CameraStore::SetKeyDown(Key key, bool down) -> void
    {
        m_Keys[static_cast<std::size_t>(key)].store(down, std::memory_order_relaxed);
    }

    auto CameraStore::ResetKeys() -> void
    {
        for (auto& key : m_Keys) key.store(false, std::memory_order_relaxed);
    }

    auto CameraStore::AccumulateMouseDelta(float dx, float dy) -> void
    {
        m_MouseDeltaX.fetch_add(dx, std::memory_order_relaxed);
        m_MouseDeltaY.fetch_add(dy, std::memory_order_relaxed);
    }

    auto CameraStore::ConsumeMouseDelta(float& outX, float& outY) -> void
    {
        outX = m_MouseDeltaX.exchange(0.0f, std::memory_order_relaxed);
        outY = m_MouseDeltaY.exchange(0.0f, std::memory_order_relaxed);
    }
}