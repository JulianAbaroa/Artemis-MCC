export module UI.Settings.System;

import Service.Settings.State;
import Service.Settings.System;
import Service.Logs.System;
import Viewer.Options.Type;
import Viewer.Options.State;
import UI.Hotkey.State;
import std;

export namespace UI::Settings::System
{
    // Draws the settings window with the user preferences, the viewer options and the hotkeys.
    class SettingsUIService
    {
    private:
        using SettingsStore = Service::Settings::State::SettingsStore;
        using SettingsService = Service::Settings::System::SettingsService;
        using LogsService = Service::Logs::System::LogsService;

        using Group = Viewer::Options::Type::Group;
        using OptionsStore = Viewer::Options::State::OptionsStore;

        using HotkeyUIStore = UI::Hotkey::State::HotkeyUIStore;

        static constexpr float k_AnimationDuration{ 0.6f };

    public:
        SettingsUIService(SettingsStore& settingsStore, SettingsService& settingsService,
            LogsService& logsService, HotkeyUIStore& hotkeyStore,
            OptionsStore& optionsStore) :
            m_SettingsStore(settingsStore), m_SettingsService(settingsService),
            m_LogsService(logsService), m_HotkeyStore(hotkeyStore),
            m_OptionsStore(optionsStore) {}
        ~SettingsUIService() = default;

        SettingsUIService(const SettingsUIService&) = delete;
        auto operator=(const SettingsUIService&) -> SettingsUIService& = delete;

        // Draws the settings window content.
        auto Draw() -> void;

    private:
        SettingsStore& m_SettingsStore;
        SettingsService& m_SettingsService;
        LogsService& m_LogsService;
        HotkeyUIStore& m_HotkeyStore;
        OptionsStore& m_OptionsStore;

        std::string m_AnimatedPathLabel{};
        float m_AnimationStartTime{ 0.0f };

        float m_UIScalePreview{ 1.0f };
        bool m_IsScalePreviewInitialized{ false };

        // Draws the menu opacity, the UI scale and the input options.
        // note: The UI scale is applied when the slider is released.
        auto DrawUserPreferences() -> void;

        // Draws the group of every viewer option and the reset button.
        auto DrawViewerOptions() -> void;

        // Draws the flags and the scalars of a viewer option group.
        auto DrawViewerGroup(Group group) -> void;

        // Draws the table of the hotkeys.
        auto DrawHotkeysTable() -> void;

        // Draws the local storage toggle and the delete data button.
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