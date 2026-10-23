export module Service.Settings.State;

import std;

export namespace Service::Settings::State
{
    // Shared runtime settings.
    // note: Thread-safe. Flags are atomics and path strings are guarded by a mutex.
    class SettingsStore
    {
    public:
        SettingsStore() = default;
        ~SettingsStore() = default;

        // Whether user data is stored in the AppData folder.
        auto IsAppDataEnabled() const -> bool;
        auto SetAppDataEnabled(bool value) -> void;

        // Whether the launcher or any tab is open.
        auto IsMenuVisible() const -> bool;
        auto SetMenuVisible(bool value) -> void;

        // Whether menu windows are locked in place.
        auto IsMenuLocked() const -> bool;
        auto SetMenuLocked(bool value) -> void;

        // One-shot request to reset the tabs. The launcher clears it once applied.
        auto IsMenuResetPending() const -> bool;
        auto SetMenuResetPending(bool value) -> void;

        // Whether game mouse input is swallowed while the menu is visible.
        auto IsMouseFreezeEnabled() const -> bool;
        auto SetMouseFreezeEnabled(bool value) -> void;

        // Whether the launcher opens automatically on start.
        auto IsOpenUIOnStartEnabled() const -> bool;
        auto SetOpenUIOnStartEnabled(bool value) -> void;

        // Opacity of the menu windows.
        auto GetMenuAlpha() const -> float;
        auto SetMenuAlpha(float value) -> void;

        // Global UI scale factor.
        auto GetUIScale() const -> float;
        auto SetUIScale(float value) -> void;

        // Whether the logs tab follows new entries.
        auto IsLogsAutoScrollEnabled() const -> bool;
        auto SetLogsAutoScrollEnabled(bool value) -> void;

        // Folder of the DLL.
        auto GetBaseDirectory() const -> std::string;
        auto SetBaseDirectory(const std::string& directory) -> void;

        // Artemis folder inside LOCALAPPDATA. Empty until created.
        auto GetAppDataDirectory() const -> std::string;
        auto SetAppDataDirectory(const std::string& directory) -> void;

        // Full path of the log file.
        auto GetLoggerPath() const -> std::string;
        auto SetLoggerPath(const std::string& path) -> void;

        auto ClearAppDataDirectory() -> void;
        auto IsAppDataDirectoryEmpty() const -> bool;

    private:
        std::atomic<bool> m_IsAppDataEnabled{ false };
        std::atomic<bool> m_IsMenuVisible{ false };
        std::atomic<bool> m_IsMenuLocked{ false };
        std::atomic<bool> m_IsMenuResetPending{ false };
        std::atomic<bool> m_IsMouseFreezeEnabled{ true };
        std::atomic<bool> m_IsOpenUIOnStartEnabled{ false };
        std::atomic<float> m_MenuAlpha{ 1.0f };
        std::atomic<float> m_UIScale{ 1.0f };
        std::atomic<bool> m_IsLogsAutoScrollEnabled{ true };

        std::string m_BaseDirectory{};
        std::string m_AppDataDirectory{};
        std::string m_LoggerPath{};
        // Guards the three path strings.
        mutable std::mutex m_Mutex{};
    };
}