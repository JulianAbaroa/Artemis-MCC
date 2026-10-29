export module DevTools.Workbench.Hotkey.System;

import Services.Settings.State;
import Engine.Render.Window.Type;
import DevTools.Viewer.Overlay.State;
import DevTools.Workbench.Hotkey.Type;
import DevTools.Workbench.Hotkey.State;
import DevTools.Workbench.Launcher.State;
import DevTools.Workbench.Capture.State;

export namespace DevTools::Workbench::Hotkey::System
{
    // Runs the action of the hotkey pressed in the game window.
    class HotkeyUIService
    {
    private:
        using SettingsStore = Services::Settings::State::SettingsStore;

        using WindowMessage = Engine::Render::Window::Type::WindowMessage;

        using OverlayStore = DevTools::Viewer::Overlay::State::OverlayStore;

        using Action = DevTools::Workbench::Hotkey::Type::Action;
        using HotkeyUIStore = DevTools::Workbench::Hotkey::State::HotkeyUIStore;
        using LauncherUIStore = DevTools::Workbench::Launcher::State::LauncherUIStore;
        using CaptureUIStore = DevTools::Workbench::Capture::State::CaptureUIStore;

    public:
        HotkeyUIService(SettingsStore& settingsStore, HotkeyUIStore& hotkeyStore,
            LauncherUIStore& launcherStore, CaptureUIStore& captureStore, OverlayStore& overlayStore) :
            m_SettingsStore(settingsStore), m_HotkeyStore(hotkeyStore),
            m_LauncherStore(launcherStore), m_CaptureStore(captureStore), m_OverlayStore(overlayStore) {}
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
        CaptureUIStore& m_CaptureStore;
        OverlayStore& m_OverlayStore;

        // Runs the action of a hotkey.
        auto Execute(Action action) -> void;
    };
}