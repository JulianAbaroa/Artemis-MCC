export module Service.Logs.System;

import Service.Logs.Type;
import Service.Logs.State;
import Service.Settings.State;
import std;

export namespace Service::Logs::System
{
    // Formats, stores and writes log messages.
    // note: Thread-safe. Message can be called from any thread.
    class LogsService
    {
    private:
        using LogEntry = Service::Logs::Type::LogEntry;
        using LogLevel = Service::Logs::Type::LogLevel;

        using LogsStore = Service::Logs::State::LogsStore;
        using SettingsStore = Service::Settings::State::SettingsStore;

    public:
        LogsService(SettingsStore& settingsStore, LogsStore& logsStore) :
            m_SettingsStore(settingsStore), m_LogsStore(logsStore) {}
        ~LogsService() = default;

        // Formats a message, stores it and appends it to the log file.
        // note: Body syntax is "[Tag] LEVEL: text". Tag and LEVEL (ERROR, WARNING, INFO) are optional.
        template <typename... Args>
        auto Message(std::format_string<Args...> fmt, Args&&... args) -> void
        {
            std::string currentBody = std::format(fmt, std::forward<Args>(args)...);

            std::lock_guard<std::mutex> lock(m_Mutex);

            LogEntry entry;
            entry.Timestamp = this->GetTimestampString();

            this->ParseEntryTags(entry, currentBody);
            this->ParseLogLevel(entry, currentBody);

            std::string tagPart = entry.Tag.empty() ? "" : entry.Tag + " ";

            entry.FullText = entry.Timestamp + tagPart +
                entry.MessagePrefix + entry.Message;

            m_LogsStore.PushBack(entry);
            this->WriteToLogFile(entry.FullText);
        }

    private:
        SettingsStore& m_SettingsStore;
        LogsStore& m_LogsStore;

        std::mutex m_Mutex{};

        // Current local time as "YYYY-MM-DD HH:MM:SS ", with trailing space.
        auto GetTimestampString() -> std::string;

        // Moves a leading "[Tag]" from body into entry.Tag.
        auto ParseEntryTags(LogEntry& entry, std::string& body) -> void;

        // Reads the level prefix from body and fills Level, MessagePrefix and Message.
        auto ParseLogLevel(LogEntry& entry, std::string& body) -> void;

        // Appends a line to the log file. Skips silently if the file cannot be opened.
        auto WriteToLogFile(const std::string& line) -> void;
    };
}