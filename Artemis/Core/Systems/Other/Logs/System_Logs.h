#pragma once

#include "Core/Types/Other/Logs/LogTypes.h"

#include <functional>
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>

class State_Settings;
class State_Logs;

struct Sys_Logs_Deps
{
    State_Settings& State_Settings;
    State_Logs& State_Logs;
};

class System_Logs
{
public:
    System_Logs(Sys_Logs_Deps deps) : m_Deps(deps) {}
    ~System_Logs() = default;

    void Log(const char* format, ...);

    bool HasUnreadError() const;
    bool HasUnreadWarning() const;
    void ClearUnreadStates();

    std::chrono::steady_clock::time_point GetLastAlertTime() const;

	void AddLog(LogEntry entry);
	void RemoveLogsIf(std::function<bool(const LogEntry&)> predicate);

private:
    Sys_Logs_Deps m_Deps;

    std::string GetTimestampString();
    void ParseEntryTags(LogEntry& entry, std::string& body);
    void ParseLogLevel(LogEntry& entre, std::string& body);
    void UpdateAlertState(LogLevel level);
    void WriteToLogFile(const char* header, const char* message);

    std::atomic<bool> m_UnreadError{ false };
    std::atomic<bool> m_UnreadWarning{ false };
    std::chrono::steady_clock::time_point m_LastAlertTime{};
    std::mutex m_Mutex;
};