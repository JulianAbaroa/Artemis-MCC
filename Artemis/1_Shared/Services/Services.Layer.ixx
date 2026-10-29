export module Services.Layer;

import Common.Log.System;
import Services.Settings.State;
import Services.Settings.System;
import Services.Logs.State;
import Services.Logs.System;
import Services.Telemetry.State;
import Services.Telemetry.System;

export namespace Services
{
    // Root of the Service layer. Owns the stores and services shared by every other layer.
    // note: Members are built in declaration order, so each service is declared after the stores it references.
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;
        using SettingsStore = Services::Settings::State::SettingsStore;
        using SettingsService = Services::Settings::System::SettingsService;
        using LogsStore = Services::Logs::State::LogsStore;
        using LogsSink = Services::Logs::System::LogsSink;
        using TelemetryStore = Services::Telemetry::State::TelemetryStore;
        using TelemetryService = Services::Telemetry::System::TelemetryService;

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
        LogsSink m_LogsSink{ m_SettingsStore, m_LogsStore };
        Log m_Log{ m_LogsSink };
        SettingsService m_SettingsService{ m_SettingsStore, m_Log };
        TelemetryService m_TelemetryService{ m_TelemetryStore };
    };
}