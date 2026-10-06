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
    // F4 toggles the free camera, F5 toggles follow and F6 toggles the labels.
    // While the camera is active it takes the movement keys, the arrows (overlay mode and page) and the left click (pick).
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

        // return: True if the camera is active and took the movement.
        auto HandleMouse(RawMouse& mouse) -> bool;

    private:
        CameraStore& m_CameraStore;
        SelectionStore& m_SelectionStore;
        OverlayStore& m_OverlayStore;
        OptionsStore& m_OptionsStore;
        SettingsStore& m_SettingsStore;

        // return: True if the key is a hotkey. A held key does not repeat the action.
        auto HandleHotkey(const WindowMessage& message) -> bool;
    };
}