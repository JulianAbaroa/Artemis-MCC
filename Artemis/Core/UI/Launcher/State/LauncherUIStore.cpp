module UI.Launcher.State;

namespace
{
	using UI::Launcher::Type::ToIndex;
}

namespace UI::Launcher::State
{
	auto LauncherUIStore::IsVisible() const -> bool
	{
		return m_IsVisible.load(std::memory_order_relaxed);
	}

	auto LauncherUIStore::SetVisible(bool value) -> void
	{
		m_IsVisible.store(value, std::memory_order_relaxed);
	}

	auto LauncherUIStore::ToggleVisible() -> void
	{
		m_IsVisible.store(!m_IsVisible.load(std::memory_order_relaxed),
			std::memory_order_relaxed);
	}

	auto LauncherUIStore::IsTabVisible(Tab tab) const -> bool
	{
		return m_TabVisible[ToIndex(tab)].load(std::memory_order_relaxed);
	}

	auto LauncherUIStore::SetTabVisible(Tab tab, bool value) -> void
	{
		m_TabVisible[ToIndex(tab)].store(value, std::memory_order_relaxed);
	}

	auto LauncherUIStore::ToggleTab(Tab tab) -> void
	{
		auto& visible = m_TabVisible[ToIndex(tab)];
		visible.store(!visible.load(std::memory_order_relaxed), std::memory_order_relaxed);
	}

	auto LauncherUIStore::IsAnyTabVisible() const -> bool
	{
		for (const auto& visible : m_TabVisible)
		{
			if (visible.load(std::memory_order_relaxed)) return true;
		}

		return false;
	}

	auto LauncherUIStore::RequestTabReset(Tab tab) -> void
	{
		m_TabResetRequested[ToIndex(tab)].store(true, std::memory_order_relaxed);
	}

	auto LauncherUIStore::ConsumeTabReset(Tab tab) -> bool
	{
		return m_TabResetRequested[ToIndex(tab)].exchange(false, std::memory_order_relaxed);
	}

	auto LauncherUIStore::RequestToggleAllTabs() -> void
	{
		m_ToggleAllRequested.store(true, std::memory_order_relaxed);
	}

	auto LauncherUIStore::ConsumeToggleAllTabs() -> bool
	{
		return m_ToggleAllRequested.exchange(false, std::memory_order_relaxed);
	}

	auto LauncherUIStore::SaveTabSnapshot() -> void
	{
		for (std::size_t i = 0; i < k_TabCount; ++i)
		{
			m_TabSnapshot[i] = m_TabVisible[i].load(std::memory_order_relaxed);
		}

		m_HasTabSnapshot = true;
	}

	auto LauncherUIStore::HasTabSnapshot() const -> bool
	{
		return m_HasTabSnapshot;
	}

	auto LauncherUIStore::WasTabVisible(Tab tab) const -> bool
	{
		return m_TabSnapshot[ToIndex(tab)];
	}
}