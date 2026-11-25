export module DevTools.Viewer.Control.System;

import Services.Settings.State;
import Engine.Render.Window.Type;
import DevTools.Viewer.Camera.State;
import DevTools.Viewer.Director.State;
import DevTools.Viewer.Selection.State;
import DevTools.Viewer.Options.State;
import std;

export namespace DevTools::Viewer::Control::System
{
    // Turns window messages and raw mouse movement into viewer actions.
    // F4 toggles the viewer, TAB switches between the Free and the Game mode, F6 toggles the labels and the up and down arrows ask for the
    // next and previous player of the roster (VW.9b), in both modes. The director (VW.14) has F7 (play or stop), Insert (adds a fixed shot from the free
    // camera) and the right and left arrows (next and previous shot, only while it plays).
    // In Free mode the viewer takes the movement keys, Ctrl (custom speed), the left click (pick),
    // the right click (zoom), the wheel (speed and field of view) and the mouse.
    // In Game mode it takes nothing but its own hotkeys, so the game keeps its controls.
    class ControlService
    {
    private:
        using SettingsStore = Services::Settings::State::SettingsStore;
        using WindowMessage = Engine::Render::Window::Type::WindowMessage;
        using RawMouse = Engine::Render::Window::Type::RawMouse;
        using CameraStore = DevTools::Viewer::Camera::State::CameraStore;
        using DirectorStore = DevTools::Viewer::Director::State::DirectorStore;
        using SelectionStore = DevTools::Viewer::Selection::State::SelectionStore;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;

    public:
        ControlService(SettingsStore& settingsStore, CameraStore& cameraStore,
            SelectionStore& selectionStore,
            OptionsStore& optionsStore, DirectorStore& directorStore) :
            m_CameraStore(cameraStore), m_SelectionStore(selectionStore),
            m_OptionsStore(optionsStore), m_DirectorStore(directorStore),
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
        OptionsStore& m_OptionsStore;
        DirectorStore& m_DirectorStore;
        SettingsStore& m_SettingsStore;

        // Ctrl + wheel changes the custom speed. The wheel alone changes the field of view.
        auto HandleWheel(int delta) -> void;

        // return: True if the key is a hotkey. A held key does not repeat the action.
        auto HandleHotkey(const WindowMessage& message) -> bool;
    };
}