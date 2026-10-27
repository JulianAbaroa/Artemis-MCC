export module Platform.Match.System:MatchConfig;

import Common.Log.System;
import Service.Settings.State;
import Platform.Match.Type;
import Platform.Match.State;
import std;

export namespace Platform::Match::System
{
    // Library of recorded match configurations and their transplant into the engine start (D123, A15 sec. 5).
    // note: Each configuration is a file <Storage Folder>/MatchConfigs/<name>.bin. It holds the XUID of the user, so it never goes
    // to the repo.
    class MatchConfigService
    {
    private:
        using path = std::filesystem::path;

        using Log = Common::Log::System::Log;
        using SettingsStore = Service::Settings::State::SettingsStore;

        using MatchConfigStore = Platform::Match::State::MatchConfigStore;

    public:
        MatchConfigService(Log& log, SettingsStore& settingsStore, MatchConfigStore& matchConfigStore) :
            m_Log(log), m_SettingsStore(settingsStore), m_MatchConfigStore(matchConfigStore) {}
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

        // Asks the lifecycle hook to launch the match of the custom games lobby on the next frame of MCC (D124).
        // note: Without a pending configuration, the match of the lobby starts unchanged.
        auto RequestLaunch() -> void;

        // Withdraws the launch request if the lifecycle hook did not take it yet.
        auto CancelLaunch() -> void;

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
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        MatchConfigStore& m_MatchConfigStore;

        // return: The folder of the library, empty if the storage folder is not set.
        auto GetLibraryFolder() const -> path;
    };
}