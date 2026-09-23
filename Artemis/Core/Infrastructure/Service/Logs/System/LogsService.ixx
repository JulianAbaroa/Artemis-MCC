export module Service.Logs.System;

import Service.Logs.Type;
import Service.Logs.State;
import Service.Settings.State;
import std;

export namespace Service::Logs::System
{
    class LogsService
    {
    private:
        using Entry = Service::Logs::Type::Entry;
        using Level = Service::Logs::Type::Level;
        using SteadyClock = std::chrono::steady_clock;
        using SystemClock = std::chrono::system_clock;

        using SettingsStore = Service::Settings::State::SettingsStore;
        using LogsStore = Service::Logs::State::LogsStore;

    public:
        LogsService(SettingsStore& settingsStore, LogsStore& logsStore) :
            m_SettingsStore(settingsStore), m_LogsStore(logsStore) {}
        ~LogsService() = default;

        template <typename... Args>
        auto Message(std::format_string<Args...> fmt, Args&&... args) -> void
        {
            std::string currentBody = std::format(fmt, std::forward<Args>(args)...);

            std::lock_guard<std::mutex> lock(m_Mutex);

            Entry entry;
            entry.Timestamp = this->GetTimestampString();

            this->ParseEntryTags(entry, currentBody);
            this->ParseLogLevel(entry, currentBody);

            this->UpdateAlertState(entry.Level);

            std::string tagPart = entry.Tag.empty() ? "" : entry.Tag + " ";

            entry.FullText = entry.Timestamp + tagPart +
                entry.MessagePrefix + entry.Message;

            this->AddLog(entry);
            this->WriteToLogFile(entry.Timestamp.c_str(), entry.FullText.c_str());
        }

        auto HasUnreadError() const -> bool;
        auto HasUnreadWarning() const -> bool;
        auto ClearUnreadStates() -> void;

        auto GetLastAlertTime() const -> SteadyClock::time_point;

        auto AddLog(Entry entry) -> void;
        auto RemoveLogsIf(std::function<bool(const Entry&)> predicate) -> void;

    private:
        SettingsStore& m_SettingsStore;
        LogsStore& m_LogsStore;

        std::atomic<bool> m_UnreadError{ false };
        std::atomic<bool> m_UnreadWarning{ false };
        SteadyClock::time_point m_LastAlertTime{};
        std::mutex m_Mutex{};

        auto GetTimestampString() -> std::string;
        auto ParseEntryTags(Entry& entry, std::string& body) -> void;
        auto ParseLogLevel(Entry& entre, std::string& body) -> void;
        auto UpdateAlertState(Level level) -> void;
        auto WriteToLogFile(const char* header, const char* message) -> void;
    };
}