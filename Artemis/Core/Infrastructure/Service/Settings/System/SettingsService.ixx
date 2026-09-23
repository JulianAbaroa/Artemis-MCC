export module Service.Settings.System;

import Service.Settings.State;
import Service.Logs.System;

export namespace Service::Settings::System
{
	class SettingsService
	{
	private:
		using SettingsStore = Service::Settings::State::SettingsStore;
		using LogsService = Service::Logs::System::LogsService;

	public:
		SettingsService(SettingsStore& settingsStore, LogsService& logsService) :
			m_SettingsStore(settingsStore), m_LogsService(logsService) {}
		~SettingsService() = default;

		auto InitializePaths(char* buffer) -> void;

		auto SaveUseAppData() -> void;
		auto LoadUseAppData() -> void;

		auto CreateAppData() -> void;
		auto DeleteAppData() -> void;

	private:
		SettingsStore& m_SettingsStore;
		LogsService& m_LogsService;
	};
}