export module UI.Hotkey.System;

import Service.Settings.State;
import Platform.Input.Type;
import Viewer.Overlay.State;
import UI.Hotkey.Type;
import UI.Hotkey.State;
import UI.Launcher.State;

export namespace UI::Hotkey::System
{
    // Runs the action of the hotkey pressed in the game window.
    class HotkeyUIService
    {
    private:
        using SettingsStore = Service::Settings::State::SettingsStore;

        using WindowMessage = Platform::Input::Type::WindowMessage;

        using OverlayStore = Viewer::Overlay::State::OverlayStore;

        using Action = UI::Hotkey::Type::Action;
        using HotkeyUIStore = UI::Hotkey::State::HotkeyUIStore;
        using LauncherUIStore = UI::Launcher::State::LauncherUIStore;

    public:
        HotkeyUIService(SettingsStore& settingsStore, HotkeyUIStore& hotkeyStore,
            LauncherUIStore& launcherStore, OverlayStore& overlayStore) :
            m_SettingsStore(settingsStore), m_HotkeyStore(hotkeyStore),
            m_LauncherStore(launcherStore), m_OverlayStore(overlayStore) {}
        ~HotkeyUIService() = default;

        HotkeyUIService(const HotkeyUIService&) = delete;
        auto operator=(const HotkeyUIService&) -> HotkeyUIService& = delete;

        // Handles a key down message that matches a UI hotkey.
        // return: true if the message was consumed.
        auto HandleMessage(WindowMessage& message) -> bool;

    private:
        SettingsStore& m_SettingsStore;
        HotkeyUIStore& m_HotkeyStore;
        LauncherUIStore& m_LauncherStore;
        OverlayStore& m_OverlayStore;

        // Runs the action of a hotkey.
        auto Execute(Action action) -> void;
    };
}