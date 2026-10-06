export module UI.Logs.System;

import Service.Settings.State;
import Service.Logs.Type;
import Service.Logs.State;
import UI.Logs.Type;
import UI.Logs.State;
import std;

export namespace UI::Logs::System
{
    class LogsUIService
    {
    private:
        using LogEntry = Service::Logs::Type::LogEntry;
        using FilterState = UI::Logs::Type::FilterState;

        using SettingsStore = Service::Settings::State::SettingsStore;
        using LogsStore = Service::Logs::State::LogsStore;
        using LogsUIStore = UI::Logs::State::LogsUIStore;

        static constexpr float k_AnimationDuration{ 0.8f };

    public:
        LogsUIService(SettingsStore& settingsStore, LogsStore& logsStore,
            LogsUIStore& logsUIStore) :
            m_SettingsStore(settingsStore), m_LogsStore(logsStore),
            m_LogsUIStore(logsUIStore) {
        }
        ~LogsUIService() = default;

        LogsUIService(const LogsUIService&) = delete;
        LogsUIService& operator=(const LogsUIService&) = delete;

        auto Draw() -> void;

    private:
        SettingsStore& m_SettingsStore;
        LogsStore& m_LogsStore;
        LogsUIStore& m_LogsUIStore;

        auto DrawTopBar() -> FilterState;
        auto DrawSearchBar() -> void;
        auto DrawClearButton(const FilterState& filter) -> void;
        auto DrawCopyButton(const FilterState& filter) -> void;
        auto DrawHelpMarker() -> void;

        auto GetFilteredIndices(const FilterState& filter) const -> std::vector<int>;
        auto DrawScrollingRegion(const FilterState& filter) -> void;
        auto DrawLogLine(int realIndex, const LogEntry& entry, bool& isLogClicked) -> void;
        auto DrawLogMessage(const std::string& message) -> void;
        auto HandleLogInteraction(int realIndex, const LogEntry& entry, bool& isLogClicked) -> void;

        static auto Matches(const LogEntry& entry, const FilterState& filter) -> bool;
    };
}