#pragma once

#include "Core/Types/Logs/LogTypes.h"

#include <functional>
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>

class State_Settings;
class State_Logs;

struct System_Logs_Dependencies
{
    State_Settings& State_Settings;
    State_Logs& State_Logs;
};

class System_Logs
{
public:
    System_Logs(System_Logs_Dependencies dependencies) : 
        m_Deps(dependencies) {}
    ~System_Logs() = default;

    void Log(const char* format, ...);

    bool HasUnreadError() const;
    bool HasUnreadWarning() const;
    void ClearUnreadStates();

    std::chrono::steady_clock::time_point GetLastAlertTime() const;

	void AddLog(LogEntry entry);
	void RemoveLogsIf(std::function<bool(const LogEntry&)> predicate);

private:
    System_Logs_Dependencies m_Deps;

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