module Service.Settings.State;

namespace Service::Settings::State
{
    auto SettingsStore::IsAppDataEnabled() const -> bool
    {
        return m_IsAppDataEnabled.load();
    }

    auto SettingsStore::SetAppDataEnabled(bool value) -> void
    {
        m_IsAppDataEnabled.store(value);
    }

    auto SettingsStore::IsMenuVisible() const -> bool
    {
        return m_IsMenuVisible.load();
    }

    auto SettingsStore::SetMenuVisible(bool value) -> void
    {
        m_IsMenuVisible.store(value);
    }

    auto SettingsStore::IsMenuLocked() const -> bool
    {
        return m_IsMenuLocked.load();
    }

    auto SettingsStore::SetMenuLocked(bool value) -> void
    {
        m_IsMenuLocked.store(value);
    }

    auto SettingsStore::IsMenuResetPending() const -> bool
    {
        return m_IsMenuResetPending.load();
    }

    auto SettingsStore::SetMenuResetPending(bool value) -> void
    {
        m_IsMenuResetPending.store(value);
    }

    auto SettingsStore::IsMouseFreezeEnabled() const -> bool
    {
        return m_IsMouseFreezeEnabled.load();
    }

    auto SettingsStore::SetMouseFreezeEnabled(bool value) -> void
    {
        m_IsMouseFreezeEnabled.store(value);
    }

    auto SettingsStore::IsOpenUIOnStartEnabled() const -> bool
    {
        return m_IsOpenUIOnStartEnabled.load();
    }

    auto SettingsStore::SetOpenUIOnStartEnabled(bool value) -> void
    {
        m_IsOpenUIOnStartEnabled.store(value);
    }

    auto SettingsStore::GetMenuAlpha() const -> float
    {
        return m_MenuAlpha.load();
    }

    auto SettingsStore::SetMenuAlpha(float value) -> void
    {
        m_MenuAlpha.store(value);
    }

    auto SettingsStore::GetUIScale() const -> float
    {
        return m_UIScale.load();
    }

    auto SettingsStore::SetUIScale(float value) -> void
    {
        m_UIScale.store(value);
    }

    auto SettingsStore::IsLogsAutoScrollEnabled() const -> bool
    {
        return m_IsLogsAutoScrollEnabled.load();
    }

    auto SettingsStore::SetLogsAutoScrollEnabled(bool value) -> void
    {
        m_IsLogsAutoScrollEnabled.store(value);
    }

    auto SettingsStore::GetBaseDirectory() const -> std::string
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_BaseDirectory;
    }

    auto SettingsStore::SetBaseDirectory(const std::string& directory) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_BaseDirectory = directory;
    }

    auto SettingsStore::GetAppDataDirectory() const -> std::string
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_AppDataDirectory;
    }

    auto SettingsStore::SetAppDataDirectory(const std::string& directory) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_AppDataDirectory = directory;
    }

    auto SettingsStore::GetLoggerPath() const -> std::string
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_LoggerPath;
    }

    auto SettingsStore::SetLoggerPath(const std::string& path) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_LoggerPath = path;
    }

    auto SettingsStore::ClearAppDataDirectory() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_AppDataDirectory.clear();
    }

    auto SettingsStore::IsAppDataDirectoryEmpty() const -> bool
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_AppDataDirectory.empty();
    }
}