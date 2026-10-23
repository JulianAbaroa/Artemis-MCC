module;

#include <chrono>

module Service.Logs.System;

import Common.Log.System;
import std;

namespace
{
    using Common::Log::System::LevelPrefix;
}

namespace Service::Logs::System
{
    auto LogsSink::Write(const LogMessage& message) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        LogEntry entry;
        entry.Timestamp = this->GetTimestampString();
        entry.Tag = message.Tag;
        entry.MessagePrefix = LevelPrefix(message.Level);
        entry.Message = message.Message;
        entry.Level = message.Level;

        std::string tagPart = entry.Tag.empty() ? "" : entry.Tag + " ";

        entry.FullText = entry.Timestamp + tagPart +
            entry.MessagePrefix + entry.Message;

        m_LogsStore.PushBack(entry);
        this->WriteToLogFile(entry.FullText);
    }

    auto LogsSink::GetTimestampString() -> std::string
    {
        auto const now = std::chrono::system_clock::now();

        auto const localTime = std::chrono::zoned_time{
            std::chrono::current_zone(),
            std::chrono::floor<std::chrono::seconds>(now)
        };

        return std::format("{:%Y-%m-%d %H:%M:%S} ", localTime);
    }

    auto LogsSink::WriteToLogFile(const std::string& line) -> void
    {
        std::ofstream ofs(m_SettingsStore.GetLoggerPath(), std::ios::app);
        if (ofs.is_open())
        {
            ofs << line << "\n";
            ofs.close();
        }
    }
}