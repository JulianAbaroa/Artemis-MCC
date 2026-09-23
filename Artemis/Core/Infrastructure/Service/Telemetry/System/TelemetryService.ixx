export module Service.Telemetry.System;

import Service.Telemetry.State;
import std;

export namespace Service::Telemetry::System
{
	class TelemetryService
	{
	private:
		using Clock = std::chrono::steady_clock;
		using DurationDouble = std::chrono::duration<double>;

		using TelemetryStore = Service::Telemetry::State::TelemetryStore;

	public:
		TelemetryService(TelemetryStore& telemetryStore) :
			m_TelemetryStore(telemetryStore) {}
		~TelemetryService() = default;

		auto Update() -> void;

	private:
		TelemetryStore& m_TelemetryStore;

		Clock::time_point m_LastUpdate{ Clock::now() };
	};
}