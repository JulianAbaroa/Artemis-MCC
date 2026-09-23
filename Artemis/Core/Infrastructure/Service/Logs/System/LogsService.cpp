module;

#include <chrono>

module Service.Logs.System;

import std;

namespace Service::Logs::System
{
    auto LogsService::HasUnreadError() const -> bool
    {
        return m_UnreadError.load();
    }

    auto LogsService::HasUnreadWarning() const -> bool
    {
        return m_UnreadWarning.load();
    }

    auto LogsService::ClearUnreadStates() -> void
    {
        m_UnreadError.store(false);
        m_UnreadWarning.store(false);
    }


    auto LogsService::GetLastAlertTime() const -> SteadyClock::time_point
    { 
        return m_LastAlertTime; 
    }


    auto LogsService::AddLog(Entry entry) -> void
    {
        m_LogsStore.PushBack(entry);
        m_LogsStore.TrimToSize(m_LogsStore.GetMaxCapacity());
    }

    auto LogsService::RemoveLogsIf(std::function<bool(const Entry&)> predicate) -> void
    {
        m_LogsStore.RemoveIf(predicate);
    }

    auto LogsService::GetTimestampString() -> std::string
    {
        auto const now = std::chrono::system_clock::now();

        auto const localTime = std::chrono::zoned_time{
            std::chrono::current_zone(),
            std::chrono::floor<std::chrono::seconds>(now)
        };

        return std::format("{:%Y-%m-%d %H:%M:%S} ", localTime);
    }

    auto LogsService::ParseEntryTags(Entry& entry, std::string& body) -> void
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

    auto LogsService::ParseLogLevel(Entry& entry, std::string& body) -> void
    {
        if (body.find("ERROR:") == 0)
        {
            entry.Level = Level::Error;
            entry.MessagePrefix = "ERROR: ";
            entry.Message = body.substr(7);
        }
        else if (body.find("WARNING:") == 0)
        {
            entry.Level = Level::Warning;
            entry.MessagePrefix = "WARNING: ";
            entry.Message = body.substr(9);
        }
        else if (body.find("INFO:") == 0)
        {
            entry.Level = Level::Info;
            entry.MessagePrefix = "INFO: ";
            entry.Message = body.substr(6);
        }
        else
        {
            entry.Level = Level::Default;
            entry.MessagePrefix = "";
            entry.Message = body;
        }
    }

    auto LogsService::UpdateAlertState(Level level) -> void
    {
        if (level == Level::Error || level == Level::Warning)
        {
            if (level == Level::Error) m_UnreadError.store(true);
            else m_UnreadWarning.store(true);

            m_LastAlertTime = std::chrono::steady_clock::now();
        }
    }

    auto LogsService::WriteToLogFile(const char* header, const char* message) -> void
    {
        std::ofstream ofs(m_SettingsStore.GetLoggerPath(), std::ios::app);
        if (ofs.is_open())
        {
            ofs << header << message << "\n";
            ofs.close();
        }
    }
}