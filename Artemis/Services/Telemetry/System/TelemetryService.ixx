export module Service.Telemetry.System;

import Service.Telemetry.State;
import std;

export namespace Service::Telemetry::System
{
    // Turns the raw counters of TelemetryStore into per-second rates and averages.
    class TelemetryService
    {
    private:
        using steady_clock = std::chrono::steady_clock;

        using TelemetryStore = Service::Telemetry::State::TelemetryStore;

    public:
        TelemetryService(TelemetryStore& telemetryStore) :
            m_TelemetryStore(telemetryStore) {}
        ~TelemetryService() = default;

        // Converts the counters gathered since the last call into rates and averages, then resets them.
        // note: Call periodically from a single thread. Other threads keep feeding the counters meanwhile.
        auto Update() -> void;

    private:
        TelemetryStore& m_TelemetryStore;

        steady_clock::time_point m_LastUpdate{ steady_clock::now() };
    };
}