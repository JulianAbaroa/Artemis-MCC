export module UI.Launcher.System;

import Service.Settings.State;
import Platform.Render.Type;
import UI.Launcher.Type;
import UI.Launcher.State;
import UI.Icon.Type;
import std;

export namespace UI::Launcher::System
{
	class LauncherUIService
	{
	private:
		using FrameContext = Platform::Render::Type::FrameContext;
		using Tab = UI::Launcher::Type::Tab;
		using IconTexture = UI::Icon::Type::IconTexture;

		using SettingsStore = Service::Settings::State::SettingsStore;
		using LauncherUIStore = UI::Launcher::State::LauncherUIStore;

		static constexpr std::size_t k_TabCount = UI::Launcher::Type::k_TabCount;

	public:
		using TabContent = std::function<void()>;

		LauncherUIService(SettingsStore& settingsStore, LauncherUIStore& launcherStore) :
			m_SettingsStore(settingsStore), m_LauncherStore(launcherStore) {
		}
		~LauncherUIService() = default;

		LauncherUIService(const LauncherUIService&) = delete;
		LauncherUIService& operator=(const LauncherUIService&) = delete;

		auto OnInitialized() -> void;
		auto ReleaseIcons() -> void;

		auto Update() -> bool;

		auto Draw(const FrameContext& frame) -> void;
		auto DrawDockSpace() -> void;
		auto DrawTab(Tab tab, const char* title, const TabContent& content) -> void;

	private:
		SettingsStore& m_SettingsStore;
		LauncherUIStore& m_LauncherStore;

		std::array<IconTexture, k_TabCount> m_Icons{};
		bool m_AreIconsLoaded{ false };
		bool m_HasHandledOpenOnStart{ false };
		bool m_WasVisible{ false };

		auto LoadIcons(const FrameContext& frame) -> void;
		auto DrawToggleButton(Tab tab) -> void;

		auto ToggleAllTabs() -> void;
		auto ResetTabs() -> void;
	};
}