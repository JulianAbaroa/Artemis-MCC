#include "pch.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"

bool State_Settings::ShouldUseAppData() const { return m_UseAppData.load(); }
void State_Settings::SetUseAppData(bool value) { m_UseAppData.store(value); }

bool State_Settings::IsMenuVisible() const { return m_IsMenuVisible.load(); }
void State_Settings::SetMenuVisible(bool value) { m_IsMenuVisible.store(value); }

bool State_Settings::IsMenuLocked() const { return m_IsMenuLocked.load(); }
void State_Settings::SetMenuLocked(bool value) { m_IsMenuLocked.store(value); }

bool State_Settings::MustResetMenu() const { return m_MustResetMenu.load(); }
void State_Settings::SetForceMenuReset(bool value) { m_MustResetMenu.store(value); }

bool State_Settings::ShouldFreezeMouse() const { return m_ShouldFreezeMouse.load(); }
void State_Settings::SetFreezeMouse(bool value) { m_ShouldFreezeMouse.store(value); }

bool State_Settings::ShouldOpenUIOnStart() const { return m_ShouldOpenUIOnStart.load(); }
void State_Settings::SetOpenUIOnStart(bool value) { m_ShouldOpenUIOnStart.store(value); }

float State_Settings::GetMenuAlpha() const { return m_MenuAlpha.load(); }
void State_Settings::SetMenuAlpha(float value) { m_MenuAlpha.store(value); }

bool State_Settings::GetLogsAutoScroll() const { return m_LogsAutoScroll.load(); }
void State_Settings::SetLogsAutoScroll(bool value) { m_LogsAutoScroll.store(value); }

std::string State_Settings::GetBaseDirectory() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_BaseDirectory;
}

void State_Settings::SetBaseDirectory(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_BaseDirectory = directory;
}

std::string State_Settings::GetAppDataDirectory() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_AppDataDirectory;
}

void State_Settings::SetAppDataDirectory(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_AppDataDirectory = directory;
}

std::string State_Settings::GetLoggerPath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_LoggerPath;
}

void State_Settings::SetLoggerPath(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_LoggerPath = directory;
}


void State_Settings::ClearAppDataDirectory()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_AppDataDirectory.clear();
}

bool State_Settings::IsAppDataDirectoryEmpty() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_AppDataDirectory.empty();
}