#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"
#include "Core/States/Interface/State_Debug.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"
#include <fstream>

void System_Debug::Log(const char* format, ...)
{
    char messageBuffer[1024]{};
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);

    std::lock_guard<std::mutex> lock(m_Mutex);

    LogEntry entry;
    entry.Timestamp = this->GetTimestampString();
    std::string currentBody = messageBuffer;

    this->ParseEntryTags(entry, currentBody);
    this->ParseLogLevel(entry, currentBody);

    this->UpdateAlertState(entry.Level);

    std::string tagPart = entry.Tag.empty() ? "" : entry.Tag + " ";
    entry.FullText = entry.Timestamp + tagPart + entry.MessagePrefix + entry.Message;

    g_pSystem->Debug->AddLog(entry);
    this->WriteToLogFile(entry.Timestamp.c_str(), messageBuffer);
}


bool System_Debug::HasUnreadError() const { return m_UnreadError.load(); }
bool System_Debug::HasUnreadWarning() const { return m_UnreadWarning.load(); }

void System_Debug::ClearUnreadStates()
{
    m_UnreadError.store(false);
    m_UnreadWarning.store(false);
}


std::chrono::steady_clock::time_point System_Debug::GetLastAlertTime() const { return m_LastAlertTime; }


void System_Debug::AddLog(LogEntry entry)
{
    g_pState->Debug->PushBack(entry);
    g_pState->Debug->TrimToSize(g_pState->Debug->GetMaxCapacity());
}

void System_Debug::RemoveLogsIf(std::function<bool(const LogEntry&)> predicate)
{
    g_pState->Debug->RemoveIf(predicate);
}


std::string System_Debug::GetTimestampString()
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    auto pad = [](int val) { return (val < 10 ? "0" : "") + std::to_string(val); };

    return std::to_string(st.wYear) + "-" +
        pad(st.wMonth) + "-" +
        pad(st.wDay) + " " +
        pad(st.wHour) + ":" +
        pad(st.wMinute) + ":" +
        pad(st.wSecond) + " ";
}

void System_Debug::ParseEntryTags(LogEntry& entry, std::string& body)
{
    size_t tagStart = body.find('[');
    size_t tagEnd = body.find(']', tagStart);

    if (tagStart != std::string::npos && tagEnd != std::string::npos)
    {
        entry.Tag = body.substr(tagStart, tagEnd - tagStart + 1);
        body = body.substr(tagEnd + 1);

        if (!body.empty() && body[0] == ' ') body.erase(0, 1);
    }
    else
    {
        entry.Tag = "";
    }
}

void System_Debug::ParseLogLevel(LogEntry& entry, std::string& body)
{
    if (body.find("ERROR:") == 0)
    {
        entry.Level = LogLevel::Error;
        entry.MessagePrefix = "ERROR: ";
        entry.Message = body.substr(7);
    }
    else if (body.find("WARNING:") == 0)
    {
        entry.Level = LogLevel::Warning;
        entry.MessagePrefix = "WARNING: ";
        entry.Message = body.substr(9);
    }
    else if (body.find("INFO:") == 0)
    {
        entry.Level = LogLevel::Info;
        entry.MessagePrefix = "INFO: ";
        entry.Message = body.substr(6);
    }
    else
    {
        entry.Level = LogLevel::Default;
        entry.MessagePrefix = "";
        entry.Message = body;
    }
}

void System_Debug::UpdateAlertState(LogLevel level)
{
    if (level == LogLevel::Error || level == LogLevel::Warning)
    {
        if (level == LogLevel::Error) m_UnreadError.store(true);
        else m_UnreadWarning.store(true);

        m_LastAlertTime = std::chrono::steady_clock::now();
    }
}

void System_Debug::WriteToLogFile(const char* header, const char* message)
{
    std::ofstream ofs(g_pState->Infrastructure->Settings->GetLoggerPath(), std::ios::app);
    if (ofs.is_open())
    {
        ofs << header << message << "\n";
        ofs.close();
    }
}