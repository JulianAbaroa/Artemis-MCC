module UI.Overlay.State;

namespace UI::Overlay::State
{
	auto OverlayUIStore::IsVisible() const -> bool
	{
		return m_IsVisible.load(std::memory_order_relaxed);
	}

	auto OverlayUIStore::SetVisible(bool value) -> void
	{
		m_IsVisible.store(value, std::memory_order_relaxed);
	}

	auto OverlayUIStore::ToggleVisible() -> void
	{
		m_IsVisible.store(!m_IsVisible.load(std::memory_order_relaxed),
			std::memory_order_relaxed);
	}
}