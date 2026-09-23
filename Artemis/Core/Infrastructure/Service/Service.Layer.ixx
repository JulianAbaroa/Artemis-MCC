export module Service.Layer;

import Service.Settings.State;
import Service.Settings.System;
import Service.Logs.State;
import Service.Logs.System;
import Service.Telemetry.State;
import Service.Telemetry.System;

export namespace Service
{
	class Layer
	{
	private:
		using SettingsStore = Service::Settings::State::SettingsStore;
		using LogsStore = Service::Logs::State::LogsStore;
		using TelemetryStore = Service::Telemetry::State::TelemetryStore;

		using LogsService = Service::Logs::System::LogsService;
		using SettingsService = Service::Settings::System::SettingsService;
		using TelemetryService = Service::Telemetry::System::TelemetryService;

	public:
		Layer() = default;
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		SettingsStore m_SettingsStore;
		LogsStore m_LogsStore;
		TelemetryStore m_TelemetryStore;

		// --- System ---
		LogsService m_LogsService{ m_SettingsStore, m_LogsStore };
		SettingsService m_SettingsService{ m_SettingsStore, m_LogsService };
		TelemetryService m_TelemetryService{ m_TelemetryStore };
	};
}