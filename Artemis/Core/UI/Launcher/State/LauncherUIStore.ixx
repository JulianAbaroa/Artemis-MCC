export module UI.Launcher.State;

import UI.Launcher.Type;
import std;

export namespace UI::Launcher::State
{
	class LauncherUIStore
	{
	private:
		using Tab = UI::Launcher::Type::Tab;

		static constexpr std::size_t k_TabCount = UI::Launcher::Type::k_TabCount;

	public:
		LauncherUIStore() = default;
		~LauncherUIStore() = default;

		LauncherUIStore(const LauncherUIStore&) = delete;
		LauncherUIStore& operator=(const LauncherUIStore&) = delete;

		auto IsVisible() const -> bool;
		auto SetVisible(bool value) -> void;
		auto ToggleVisible() -> void;

		auto IsTabVisible(Tab tab) const -> bool;
		auto SetTabVisible(Tab tab, bool value) -> void;
		auto ToggleTab(Tab tab) -> void;
		auto IsAnyTabVisible() const -> bool;

		auto RequestTabReset(Tab tab) -> void;
		auto ConsumeTabReset(Tab tab) -> bool;

		auto RequestToggleAllTabs() -> void;
		auto ConsumeToggleAllTabs() -> bool;

		auto SaveTabSnapshot() -> void;
		auto HasTabSnapshot() const -> bool;
		auto WasTabVisible(Tab tab) const -> bool;

	private:
		std::atomic<bool> m_IsVisible{ false };

		std::array<std::atomic<bool>, k_TabCount> m_TabVisible{};
		std::array<std::atomic<bool>, k_TabCount> m_TabResetRequested{};

		std::atomic<bool> m_ToggleAllRequested{ false };

		std::array<bool, k_TabCount> m_TabSnapshot{};
		bool m_HasTabSnapshot{ false };
	};
}