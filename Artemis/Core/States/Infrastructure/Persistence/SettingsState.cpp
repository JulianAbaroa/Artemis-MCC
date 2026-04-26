#include "pch.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"

bool SettingsState::ShouldUseAppData() const { return m_UseAppData.load(); }
void SettingsState::SetUseAppData(bool value) { m_UseAppData.store(value); }

bool SettingsState::IsMenuVisible() const { return m_IsMenuVisible.load(); }
void SettingsState::SetMenuVisible(bool value) { m_IsMenuVisible.store(value); }

bool SettingsState::MustResetMenu() const { return m_MustResetMenu.load(); }
void SettingsState::SetForceMenuReset(bool value) { m_MustResetMenu.store(value); }

bool SettingsState::ShouldFreezeMouse() const { return m_ShouldFreezeMouse.load(); }
void SettingsState::SetFreezeMouse(bool value) { m_ShouldFreezeMouse.store(value); }

bool SettingsState::ShouldOpenUIOnStart() const { return m_ShouldOpenUIOnStart.load(); }
void SettingsState::SetOpenUIOnStart(bool value) { m_ShouldOpenUIOnStart.store(value); }

float SettingsState::GetMenuAlpha() const { return m_MenuAlpha.load(); }
void SettingsState::SetMenuAlpha(float value) { m_MenuAlpha.store(value); }

bool SettingsState::GetLogsAutoScroll() const { return m_LogsAutoScroll.load(); }
void SettingsState::SetLogsAutoScroll(bool value) { m_LogsAutoScroll.store(value); }

std::string SettingsState::GetBaseDirectory() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_BaseDirectory;
}

void SettingsState::SetBaseDirectory(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_BaseDirectory = directory;
}

std::string SettingsState::GetAppDataDirectory() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_AppDataDirectory;
}

void SettingsState::SetAppDataDirectory(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_AppDataDirectory = directory;
}

std::string SettingsState::GetLoggerPath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_LoggerPath;
}

void SettingsState::SetLoggerPath(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_LoggerPath = directory;
}


void SettingsState::ClearAppDataDirectory()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_AppDataDirectory.clear();
}

bool SettingsState::IsAppDataDirectoryEmpty() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_AppDataDirectory.empty();
}