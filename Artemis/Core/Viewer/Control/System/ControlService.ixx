export module Viewer.Control.System;

import Service.Settings.State;
import Platform.Input.Type;
import Viewer.Camera.State;
import Viewer.Selection.State;
import Viewer.Overlay.State;
import Viewer.Options.State;
import std;

export namespace Viewer::Control::System
{
    // Turns window messages and raw mouse movement into viewer actions.
    // F4 toggles the viewer, TAB switches between the Free and the Game mode, F5 toggles follow and F6 toggles the labels.
    // In Free mode the viewer takes the movement keys, Ctrl (custom speed), the arrows (overlay mode and page), the left click (pick),
    // the right click (zoom), the wheel (speed and field of view) and the mouse.
    // In Game mode it takes nothing but its own hotkeys, so the game keeps its controls.
    class ControlService
    {
    private:
        using SettingsStore = Service::Settings::State::SettingsStore;
        using WindowMessage = Platform::Input::Type::WindowMessage;
        using RawMouse = Platform::Input::Type::RawMouse;
        using CameraStore = Viewer::Camera::State::CameraStore;
        using SelectionStore = Viewer::Selection::State::SelectionStore;
        using OverlayStore = Viewer::Overlay::State::OverlayStore;
        using OptionsStore = Viewer::Options::State::OptionsStore;

    public:
        ControlService(SettingsStore& settingsStore, CameraStore& cameraStore,
            SelectionStore& selectionStore, OverlayStore& overlayStore,
            OptionsStore& optionsStore) :
            m_CameraStore(cameraStore), m_SelectionStore(selectionStore),
            m_OverlayStore(overlayStore), m_OptionsStore(optionsStore),
            m_SettingsStore(settingsStore) {}
        ~ControlService() = default;

        ControlService(const ControlService&) = delete;
        auto operator=(const ControlService&) -> ControlService& = delete;

        // return: True if the message was consumed. Nothing is consumed while the menu is visible.
        auto HandleMessage(WindowMessage& message) -> bool;

        // return: True if the camera is active and took the movement. Nothing is consumed while the menu is visible.
        auto HandleMouse(RawMouse& mouse) -> bool;

    private:
        CameraStore& m_CameraStore;
        SelectionStore& m_SelectionStore;
        OverlayStore& m_OverlayStore;
        OptionsStore& m_OptionsStore;
        SettingsStore& m_SettingsStore;

        // Ctrl + wheel changes the custom speed and the wheel alone changes the field of view of the current zoom state.
        auto HandleWheel(int delta) -> void;

        // return: True if the key is a hotkey. A held key does not repeat the action.
        auto HandleHotkey(const WindowMessage& message) -> bool;
    };
}