export module Platform.Lifecycle.System:MatchConfig;

import Service.Logs.System;
import Service.Settings.State;
import Platform.Lifecycle.Type;
import Platform.Lifecycle.State;
import std;

export namespace Platform::Lifecycle::System
{
    // Library of recorded match configurations and their transplant into the engine start (D123, A15 sec. 5).
    // note: Each configuration is a file <Storage Folder>/MatchConfigs/<name>.bin. It holds the XUID of the user, so it never goes
    // to the repo.
    class MatchConfigService
    {
    private:
        using path = std::filesystem::path;

        using LogsService = Service::Logs::System::LogsService;
        using SettingsStore = Service::Settings::State::SettingsStore;

        using MatchConfigStore = Platform::Lifecycle::State::MatchConfigStore;

    public:
        MatchConfigService(LogsService& logsService, SettingsStore& settingsStore, MatchConfigStore& matchConfigStore) :
            m_LogsService(logsService), m_SettingsStore(settingsStore), m_MatchConfigStore(matchConfigStore) {}
        ~MatchConfigService() = default;

        MatchConfigService(const MatchConfigService&) = delete;
        auto operator=(const MatchConfigService&) -> MatchConfigService& = delete;

        // return: Names of the recorded configurations, sorted. Empty if the storage folder is not set.
        // note: Reads the folder. Called on a user action, not every frame.
        auto ListConfigs() const -> std::vector<std::string>;

        // Reads a recorded configuration and makes it the pending one.
        // return: False if the name is not valid, or the file cannot be read or has the wrong size.
        auto LoadPending(std::string_view name) -> bool;

        // Clears the pending configuration, so the next start is the match of MCC unchanged.
        auto ClearPending() -> void;

        // Arms the record of the next engine start under a name. A configuration with the same name is replaced.
        // return: False if the name is empty, too long, a device name of Windows (CON, NUL, COM1...) or holds characters other than
        // letters, digits, spaces, '_', '-' and '.'.
        auto ArmRecord(std::string_view name) -> bool;

        // Disarms the record of the next engine start.
        auto CancelRecord() -> void;

        // Writes the configuration of MCC to the library if a record is armed, and disarms it.
        // note: Called by the start hook on the MCC thread, before the transplant, so it records the match of the lobby. Throws only
        // when out of memory.
        auto Record(const std::byte* configuration) -> void;

        // Takes the pending configuration and builds the configuration the engine starts with: the one of MCC with the blocks of the
        // pending one over it (k_TransplantBlocks).
        // param start: Out. Configuration the engine starts with. Left untouched if nothing is pending.
        // return: False if nothing is pending.
        // note: Called by the start hook on the MCC thread. Throws only when out of memory.
        auto Transplant(const std::byte* configuration, std::vector<std::byte>& start) -> bool;

    private:
        LogsService& m_LogsService;
        SettingsStore& m_SettingsStore;
        MatchConfigStore& m_MatchConfigStore;

        // return: The folder of the library, empty if the storage folder is not set.
        auto GetLibraryFolder() const -> path;
    };
}