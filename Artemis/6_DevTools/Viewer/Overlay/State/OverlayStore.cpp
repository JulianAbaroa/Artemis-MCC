module DevTools.Viewer.Overlay.State;

import std;

namespace DevTools::Viewer::Overlay::State
{
    auto OverlayStore::IsVisible() const -> bool
    {
        return m_IsVisible.load(std::memory_order_relaxed);
    }

    auto OverlayStore::SetVisible(bool value) -> void
    {
        m_IsVisible.store(value, std::memory_order_relaxed);
    }

    auto OverlayStore::ToggleVisible() -> void
    {
        m_IsVisible.store(!m_IsVisible.load(std::memory_order_relaxed),
            std::memory_order_relaxed);
    }
}