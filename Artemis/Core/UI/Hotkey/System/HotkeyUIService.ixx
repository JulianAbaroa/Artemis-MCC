export module UI.Hotkey.System;

import Service.Settings.State;
import Platform.Input.Type;
import UI.Hotkey.Type;
import UI.Hotkey.State;
import UI.Launcher.State;
import Viewer.Overlay.State;

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
		using OverlayStore = Viewer::Overlay::State::OverlayStore;

	public:
		HotkeyUIService(SettingsStore& settingsStore, HotkeyUIStore& hotkeyStore,
			LauncherUIStore& launcherStore, OverlayStore& overlayStore) :
			m_SettingsStore(settingsStore), m_HotkeyStore(hotkeyStore),
			m_LauncherStore(launcherStore), m_OverlayStore(overlayStore) {}
		~HotkeyUIService() = default;

		HotkeyUIService(const HotkeyUIService&) = delete;
		HotkeyUIService& operator=(const HotkeyUIService&) = delete;

		auto HandleMessage(WindowMessage& message) -> bool;

	private:
		SettingsStore& m_SettingsStore;
		HotkeyUIStore& m_HotkeyStore;
		LauncherUIStore& m_LauncherStore;
		OverlayStore& m_OverlayStore;

		auto Execute(Action action) -> void;
	};
}