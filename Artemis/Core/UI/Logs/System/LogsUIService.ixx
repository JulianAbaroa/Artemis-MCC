export module UI.Logs.System;

import Service.Settings.State;
import Service.Logs.Type;
import Service.Logs.State;
import UI.Logs.Type;
import UI.Logs.State;
import std;

export namespace UI::Logs::System
{
    // Draws the logs window with the search, the selection and the copy actions.
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
            m_LogsUIStore(logsUIStore) {}
        ~LogsUIService() = default;

        LogsUIService(const LogsUIService&) = delete;
        auto operator=(const LogsUIService&) -> LogsUIService& = delete;

        // Draws the content of the logs window.
        auto Draw() -> void;

    private:
        SettingsStore& m_SettingsStore;
        LogsStore& m_LogsStore;
        LogsUIStore& m_LogsUIStore;

        // Draws the search, the buttons and the auto scroll option.
        // return: The filter built from the search text.
        auto DrawTopBar() -> FilterState;

        // Draws the search input and its clear button.
        auto DrawSearchBar() -> void;

        // Draws the button that removes the logs, only the filtered ones when filtering.
        auto DrawClearButton(const FilterState& filter) -> void;

        // Draws the button that copies the logs, only the filtered ones when filtering.
        auto DrawCopyButton(const FilterState& filter) -> void;

        // Draws the help icon with the controls tooltip.
        auto DrawHelpMarker() -> void;

        // Returns the indices of the logs that match the filter.
        auto GetFilteredIndices(const FilterState& filter) const -> std::vector<int>;

        // Draws the visible logs and handles the auto scroll and the background click.
        auto DrawScrollingRegion(const FilterState& filter) -> void;

        // Draws a log line with its selection.
        // param isLogClicked: Set to true if the line was clicked.
        auto DrawLogLine(int realIndex, const LogEntry& entry, bool& isLogClicked) -> void;

        // Draws the message and highlights its first quoted fragment.
        auto DrawLogMessage(const std::string& message) -> void;

        // Handles the selection clicks and the copy animation of a line.
        // param isLogClicked: Set to true if the line was clicked.
        auto HandleLogInteraction(int realIndex, const LogEntry& entry, bool& isLogClicked) -> void;

        // Checks whether the log text contains the query of the filter.
        static auto Matches(const LogEntry& entry, const FilterState& filter) -> bool;
    };
}