export module Service.Settings.System;

import Service.Settings.State;
import Service.Logs.System;
import std;

export namespace Service::Settings::System
{
    // Resolves Artemis paths and manages the AppData folder and its config flag.
    class SettingsService
    {
    private:
        using SettingsStore = Service::Settings::State::SettingsStore;
        using LogsService = Service::Logs::System::LogsService;

    public:
        SettingsService(SettingsStore& settingsStore, LogsService& logsService) :
            m_SettingsStore(settingsStore), m_LogsService(logsService) {}
        ~SettingsService() = default;

        // Sets the base directory and log path, then loads the AppData flag and creates the folder if enabled.
        // param directory: Directory of the DLL.
        auto InitializePaths(const std::string& directory) -> void;

        // Writes the AppData flag to config.ini next to the DLL.
        auto SaveAppDataEnabled() -> void;

        // Creates the Artemis folder inside LOCALAPPDATA and stores its path.
        // note: Does nothing if AppData is disabled or the folder is already set.
        auto CreateAppData() -> void;

        // Deletes the AppData folder and clears its stored path.
        auto DeleteAppData() -> void;

    private:
        SettingsStore& m_SettingsStore;
        LogsService& m_LogsService;

        // Reads the AppData flag from config.ini. Defaults to false if missing.
        auto LoadAppDataEnabled() -> void;
    };
}