module Service.Settings.State;

namespace Service::Settings::State
{
	auto SettingsStore::ShouldUseAppData() const -> bool
	{
		return m_UseAppData.load();
	}

	auto SettingsStore::SetUseAppData(bool value) -> void
	{
		m_UseAppData.store(value);
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

	auto SettingsStore::MustResetMenu() const -> bool
	{
		return m_MustResetMenu.load();
	}

	auto SettingsStore::SetForceMenuReset(bool value) -> void
	{
		m_MustResetMenu.store(value);
	}

	auto SettingsStore::ShouldFreezeMouse() const -> bool
	{
		return m_ShouldFreezeMouse.load();
	}

	auto SettingsStore::SetFreezeMouse(bool value) -> void
	{
		m_ShouldFreezeMouse.store(value);
	}

	auto SettingsStore::ShouldOpenUIOnStart() const -> bool
	{
		return m_ShouldOpenUIOnStart.load();
	}

	auto SettingsStore::SetOpenUIOnStart(bool value) -> void
	{
		m_ShouldOpenUIOnStart.store(value);
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

	auto SettingsStore::GetLogsAutoScroll() const -> bool
	{
		return m_LogsAutoScroll.load();
	}

	auto SettingsStore::SetLogsAutoScroll(bool value) -> void
	{
		m_LogsAutoScroll.store(value);
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

	auto SettingsStore::SetLoggerPath(const std::string& directory) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_LoggerPath = directory;
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