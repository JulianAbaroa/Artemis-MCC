export module Service.Layer;

import Service.Settings.State;
import Service.Settings.System;
import Service.Logs.State;
import Service.Logs.System;
import Service.Telemetry.State;
import Service.Telemetry.System;

export namespace Service
{
    // Root of the Service layer. Owns the stores and services shared by every other layer.
    // note: Members are built in declaration order, so each service is declared after the stores it references.
    class Layer
    {
    private:
        using SettingsStore = Service::Settings::State::SettingsStore;
        using SettingsService = Service::Settings::System::SettingsService;
        using LogsStore = Service::Logs::State::LogsStore;
        using LogsService = Service::Logs::System::LogsService;
        using TelemetryStore = Service::Telemetry::State::TelemetryStore;
        using TelemetryService = Service::Telemetry::System::TelemetryService;

    public:
        Layer() = default;
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        SettingsStore m_SettingsStore{};
        LogsStore m_LogsStore{};
        TelemetryStore m_TelemetryStore{};

        // --- System ---
        LogsService m_LogsService{ m_SettingsStore, m_LogsStore };
        SettingsService m_SettingsService{ m_SettingsStore, m_LogsService };
        TelemetryService m_TelemetryService{ m_TelemetryStore };
    };
}