export module UI.Logs.System;

import Service.Settings.State;
import Service.Logs.Type;
import Service.Logs.State;
import Service.Logs.System;
import UI.Logs.Type;
import UI.Logs.State;
import std;

export namespace UI::Logs::System
{
	class LogsUIService
	{
	private:
		using Entry = Service::Logs::Type::Entry;
		using FilterState = UI::Logs::Type::FilterState;

		using SettingsStore = Service::Settings::State::SettingsStore;
		using LogsStore = Service::Logs::State::LogsStore;
		using LogsService = Service::Logs::System::LogsService;
		using LogsUIStore = UI::Logs::State::LogsUIStore;

		static constexpr float k_AnimationDuration{ 0.8f };

	public:
		LogsUIService(SettingsStore& settingsStore, LogsStore& logsStore,
			LogsService& logsService, LogsUIStore& logsUIStore) :
			m_SettingsStore(settingsStore), m_LogsStore(logsStore),
			m_LogsService(logsService), m_LogsUIStore(logsUIStore) {
		}
		~LogsUIService() = default;

		LogsUIService(const LogsUIService&) = delete;
		LogsUIService& operator=(const LogsUIService&) = delete;

		auto Draw() -> void;

	private:
		SettingsStore& m_SettingsStore;
		LogsStore& m_LogsStore;
		LogsService& m_LogsService;
		LogsUIStore& m_LogsUIStore;

		auto DrawTopBar() -> FilterState;
		auto DrawSearchBar() -> void;
		auto DrawClearButton(const FilterState& filter) -> void;
		auto DrawCopyButton(const FilterState& filter) -> void;
		auto DrawHelpMarker() -> void;

		auto GetFilteredIndices(const FilterState& filter) const -> std::vector<int>;
		auto DrawScrollingRegion(const FilterState& filter) -> void;
		auto DrawLogLine(int realIndex, const Entry& entry, bool& isLogClicked) -> void;
		auto DrawLogMessage(const std::string& message) -> void;
		auto HandleLogInteraction(int realIndex, const Entry& entry, bool& isLogClicked) -> void;

		static auto Matches(const Entry& entry, const FilterState& filter) -> bool;
	};
}