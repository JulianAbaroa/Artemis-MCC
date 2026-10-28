export module Service.Logs.System;

import Common.Log.Type;
import Service.Logs.Type;
import Service.Logs.State;
import Service.Settings.State;
import std;

namespace
{
    using Common::Log::Type::LogSink;
}

export namespace Service::Logs::System
{
    // Log sink of the DLL. Stamps each message, keeps it in the LogsStore for the UI and appends it to the log file.
    // note: Thread-safe. Write can be called from any thread.
    class LogsSink : public LogSink
    {
    private:
        using LogMessage = Common::Log::Type::LogMessage;

        using LogEntry = Service::Logs::Type::LogEntry;
        using LogsStore = Service::Logs::State::LogsStore;
        using SettingsStore = Service::Settings::State::SettingsStore;

    public:
        LogsSink(SettingsStore& settingsStore, LogsStore& logsStore) :
            m_SettingsStore(settingsStore), m_LogsStore(logsStore) {}
        ~LogsSink() override = default;

        // Builds the entry of a message, stores it and appends it to the log file.
        auto Write(const LogMessage& message) -> void override;

    private:
        SettingsStore& m_SettingsStore;
        LogsStore& m_LogsStore;

        std::mutex m_Mutex{};

        // Current local time as "YYYY-MM-DD HH:MM:SS ", with trailing space.
        auto GetTimestampString() -> std::string;

        // Appends a line to the log file. Skips silently if the file cannot be opened.
        auto WriteToLogFile(const std::string& line) -> void;
    };
}