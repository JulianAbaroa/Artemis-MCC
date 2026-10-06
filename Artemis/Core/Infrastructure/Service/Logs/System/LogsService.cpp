module;

#include <chrono>

module Service.Logs.System;

import std;

namespace Service::Logs::System
{
    auto LogsService::GetTimestampString() -> std::string
    {
        auto const now = std::chrono::system_clock::now();

        auto const localTime = std::chrono::zoned_time{
            std::chrono::current_zone(),
            std::chrono::floor<std::chrono::seconds>(now)
        };

        return std::format("{:%Y-%m-%d %H:%M:%S} ", localTime);
    }

    auto LogsService::ParseEntryTags(LogEntry& entry, std::string& body) -> void
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

    auto LogsService::ParseLogLevel(LogEntry& entry, std::string& body) -> void
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

    auto LogsService::WriteToLogFile(const std::string& line) -> void
    {
        std::ofstream ofs(m_SettingsStore.GetLoggerPath(), std::ios::app);
        if (ofs.is_open())
        {
            ofs << line << "\n";
            ofs.close();
        }
    }
}