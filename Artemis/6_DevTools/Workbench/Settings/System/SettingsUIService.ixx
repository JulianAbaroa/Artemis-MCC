export module DevTools.Workbench.Settings.System;

import Common.Log.System;
import Services.Settings.State;
import Services.Settings.System;
import Services.Preferences.System;
import DevTools.Gui.Style.Type;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Camera.State;
import DevTools.Workbench.Hotkey.State;
import std;

namespace
{
    using DevTools::Gui::Style::Type::k_AnimationDuration;
}

export namespace DevTools::Workbench::Settings::System
{
    // Draws the settings window with the user preferences, the viewer options and the hotkeys.
    class SettingsUIService
    {
    private:
        using Log = Common::Log::System::Log;
        using SettingsStore = Services::Settings::State::SettingsStore;
        using SettingsService = Services::Settings::System::SettingsService;
        using PreferencesService = Services::Preferences::System::PreferencesService;

        using Group = DevTools::Viewer::Options::Type::Group;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;
        using CameraStore = DevTools::Viewer::Camera::State::CameraStore;

        using HotkeyUIStore = DevTools::Workbench::Hotkey::State::HotkeyUIStore;

    public:
        SettingsUIService(SettingsStore& settingsStore, SettingsService& settingsService,
            Log& log, HotkeyUIStore& hotkeyStore,
            OptionsStore& optionsStore, PreferencesService& preferencesService,
            CameraStore& cameraStore) :
            m_SettingsStore(settingsStore), m_SettingsService(settingsService),
            m_Log(log), m_HotkeyStore(hotkeyStore),
            m_OptionsStore(optionsStore), m_PreferencesService(preferencesService),
            m_CameraStore(cameraStore) {}
        ~SettingsUIService() = default;

        SettingsUIService(const SettingsUIService&) = delete;
        auto operator=(const SettingsUIService&) -> SettingsUIService& = delete;

        // Draws the settings window content.
        auto Draw() -> void;

    private:
        SettingsStore& m_SettingsStore;
        SettingsService& m_SettingsService;
        Log& m_Log;
        HotkeyUIStore& m_HotkeyStore;
        OptionsStore& m_OptionsStore;
        PreferencesService& m_PreferencesService;
        CameraStore& m_CameraStore;

        // ImGui time until which the save button shows its confirmation.
        double m_SaveFeedbackUntil{ 0.0 };

        std::string m_AnimatedPathLabel{};
        float m_AnimationStartTime{ 0.0f };

        float m_UIScalePreview{ 1.0f };
        bool m_IsScalePreviewInitialized{ false };

        // Draws the menu opacity, the UI scale and the input options.
        // note: The UI scale is applied when the slider is released.
        auto DrawUserPreferences() -> void;

        // Draws the group of every viewer option and the reset button.
        auto DrawViewerOptions() -> void;

        // Draws the camera state: whether the viewer is active, its mode, the follow mode and the zoom. Called inside the box of the Camera group.
        // note: These are runtime states and are not saved with the preferences.
        auto DrawCameraControls() -> void;

        // Draws the flags and the scalars of a viewer option group.
        auto DrawViewerGroup(Group group) -> void;

        // Draws the table of the hotkeys.
        auto DrawHotkeysTable() -> void;

        // Draws the local storage toggle, the save preferences button and the delete data button.
        auto DrawDataPersistence() -> void;

        // Draws the paths used by Artemis.
        auto DrawSystemDirectories() -> void;

        // Draws a row of the hotkeys table.
        auto DrawHotkeyRow(const char* label, const char* keys, const char* tooltip) -> void;

        // Draws a read only path with a right click copy.
        auto DrawPathField(const char* label, const std::string& path,
            float widthOffset = 10.0f) -> void;

        // Draws the confirmation popups of the data persistence.
        auto DrawPersistencePopups() -> void;

        // Draws the confirmation to disable the local storage.
        auto DrawConfirmDisableAppData() -> void;

        // Draws the confirmation to delete all the stored data.
        auto DrawDeleteAllAppData() -> void;
    };
}