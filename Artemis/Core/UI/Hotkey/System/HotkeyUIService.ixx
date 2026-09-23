export module UI.Hotkey.System;

import Service.Settings.State;
import Platform.Input.Type;
import UI.Hotkey.Type;
import UI.Hotkey.State;
import UI.Launcher.State;
import UI.Overlay.State;

export namespace UI::Hotkey::System
{
	class HotkeyUIService
	{
	private:
		using WindowMessage = Platform::Input::Type::WindowMessage;
		using Action = UI::Hotkey::Type::Action;

		using SettingsStore = Service::Settings::State::SettingsStore;
		using HotkeyUIStore = UI::Hotkey::State::HotkeyUIStore;
		using LauncherUIStore = UI::Launcher::State::LauncherUIStore;
		using OverlayUIStore = UI::Overlay::State::OverlayUIStore;

	public:
		HotkeyUIService(SettingsStore& settingsStore, HotkeyUIStore& hotkeyStore,
			LauncherUIStore& launcherStore, OverlayUIStore& overlayVisibilityStore) :
			m_SettingsStore(settingsStore), m_HotkeyStore(hotkeyStore),
			m_LauncherStore(launcherStore), m_OverlayVisibilityStore(overlayVisibilityStore) {
		}
		~HotkeyUIService() = default;

		HotkeyUIService(const HotkeyUIService&) = delete;
		HotkeyUIService& operator=(const HotkeyUIService&) = delete;

		auto HandleMessage(WindowMessage& message) -> bool;

	private:
		SettingsStore& m_SettingsStore;
		HotkeyUIStore& m_HotkeyStore;
		LauncherUIStore& m_LauncherStore;
		OverlayUIStore& m_OverlayVisibilityStore;

		auto Execute(Action action) -> void;
	};
}