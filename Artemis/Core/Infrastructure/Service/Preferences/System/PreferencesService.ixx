export module Service.Preferences.System;

import Service.Preferences.Type;
import Service.Settings.State;
import Service.Logs.System;
import std;

export namespace Service::Preferences::System
{
    // Persists user preferences to user_preferences.cfg in the AppData folder.
    // note: Does nothing unless AppData is enabled. Other modules add their keys through RegisterSection.
    class PreferencesService
    {
    private:
        using SectionSaver = Service::Preferences::Type::SectionSaver;
        using SectionLoader = Service::Preferences::Type::SectionLoader;
        using Section = Service::Preferences::Type::Section;

        using SettingsStore = Service::Settings::State::SettingsStore;
        using LogsService = Service::Logs::System::LogsService;

    public:
        PreferencesService(SettingsStore& settingsStore, LogsService& logsService) :
            m_SettingsStore(settingsStore), m_LogsService(logsService) {}
        ~PreferencesService() = default;

        // Writes all preferences to disk, replacing the file.
        auto Save() -> void;

        // Reads preferences from disk and applies them. A missing file keeps the defaults.
        auto Load() -> void;

        // Registers a group of keys owned by another module.
        // param prefix: Key prefix that identifies the group.
        // param saver: Writes the group's lines.
        // param loader: Receives each matching key without its prefix, and the value.
        auto RegisterSection(std::string prefix, SectionSaver saver, SectionLoader loader) -> void;

    private:
        SettingsStore& m_SettingsStore;
        LogsService& m_LogsService;

        std::vector<Section> m_Sections{};

        auto GetPreferencesFilePath() const -> std::string;

        // Applies one "key=value" line. Blank lines and comment lines are ignored.
        auto ParseLine(const std::string& line) -> void;

        // Writes the Settings_ keys.
        auto SaveSettingsState(std::ofstream& file) -> void;

        // Writes the UI_ keys.
        auto SaveUI(std::ofstream& file) -> void;

        // Applies one Settings_ key. Invalid numbers fall back to 1.0.
        auto LoadSettingsState(const std::string& key, const std::string& value) -> void;

        // Applies one UI_ key.
        auto LoadUI(const std::string& key, const std::string& value) -> void;
    };
}