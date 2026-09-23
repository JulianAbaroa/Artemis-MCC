export module Service.Preferences.System;

import Platform.Lifecycle.State;
import Service.Settings.State;
import Service.Logs.System;
import std;

export namespace Service::Preferences::System
{
	using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
	using SettingsStore = Service::Settings::State::SettingsStore;
	using LogsService = Service::Logs::System::LogsService;

	class PreferencesService
	{
	public:
		PreferencesService(LifecycleStore& lifecycleStore,
			SettingsStore& settingsStore, LogsService& logService) :
			m_LifecycleStore(lifecycleStore), m_SettingsStore(settingsStore),
			m_LogService(logService) {}
		~PreferencesService() = default;

		auto Save() -> void;
		auto Load() -> void;

	private:
		LifecycleStore& m_LifecycleStore;
		SettingsStore& m_SettingsStore;
		LogsService& m_LogService;

		auto GetPreferencesFilePath() const -> std::string;
		auto ParseLine(const std::string& line) -> void;

		auto SaveLifeCycleState(std::ofstream& file) -> void;
		auto SaveSettingsState(std::ofstream& file) -> void;
		auto SaveUI(std::ofstream& file) -> void;

		auto LoadLifecycleState(std::string& key, std::string& value) -> void;
		auto LoadSettingsState(std::string& key, std::string& value) -> void;
		auto LoadUI(std::string& key, std::string& value) -> void;
	};
}
