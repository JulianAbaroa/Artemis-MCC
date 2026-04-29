#pragma once

#include "Core/Types/Domain/ModTypes.h"
#include <string>
#include <atomic>
#include <vector>
#include <mutex>

class State_Settings
{
public:
	bool ShouldUseAppData() const;
	void SetUseAppData(bool value);

	bool IsMenuVisible() const;
	void SetMenuVisible(bool value);

	bool IsMenuLocked() const;
	void SetMenuLocked(bool value);

	bool MustResetMenu() const;
	void SetForceMenuReset(bool value);

	bool ShouldFreezeMouse() const;
	void SetFreezeMouse(bool value);

	bool ShouldOpenUIOnStart() const;
	void SetOpenUIOnStart(bool value);

	float GetMenuAlpha() const;
	void SetMenuAlpha(float value);

	bool GetLogsAutoScroll() const;
	void SetLogsAutoScroll(bool value);

	std::string GetBaseDirectory() const;
	void SetBaseDirectory(const std::string& directory);

	std::string GetAppDataDirectory() const;
	void SetAppDataDirectory(const std::string& directory);

	std::string GetLoggerPath() const;
	void SetLoggerPath(const std::string& directory);

	void ClearAppDataDirectory();
	bool IsAppDataDirectoryEmpty() const;

private:
	std::atomic<bool> m_UseAppData{ false };
	std::atomic<bool> m_IsMenuVisible{ true };
	std::atomic<bool> m_IsMenuLocked{ false };
	std::atomic<bool> m_MustResetMenu{ false };
	std::atomic<bool> m_ShouldFreezeMouse{ true };
	std::atomic<bool> m_ShouldOpenUIOnStart{ true };
	std::atomic<float> m_MenuAlpha{ 1.0f };
	std::atomic<bool> m_LogsAutoScroll{ true };

	std::string m_BaseDirectory{};
	std::string m_AppDataDirectory{};
	std::string m_LoggerPath{};
	mutable std::mutex m_Mutex;
};