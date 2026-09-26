export module Export.Tick.Hook;

import Service.Logs.System;
import Service.Telemetry.State;
import Platform.Memory.System;
import Platform.Lifecycle.State;
import std;

export namespace Export::Tick::Hook
{
	class SimulationTicksDetour
	{
	private:
		using SteadyClock = std::chrono::steady_clock;
		using NanoSeconds = std::chrono::nanoseconds;

		using LogsService = Service::Logs::System::LogsService;
		using TelemetryStore = Service::Telemetry::State::TelemetryStore;
		using AOBService = Platform::Memory::System::AOBService;
		using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;

	public:
		SimulationTicksDetour(LogsService& logsService, TelemetryStore& telemetryStore,
			AOBService& aobService, LifecycleStore& lifecycleStore) :
			m_LogsService(logsService), m_TelemetryStore(telemetryStore),
			m_AOBService(aobService), m_LifecycleStore(lifecycleStore) {}
		~SimulationTicksDetour() = default;

		auto Install() -> void;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		TelemetryStore& m_TelemetryStore;
		AOBService& m_AOBService;
		LifecycleStore& m_LifecycleStore;

		static SimulationTicksDetour* s_Instance;

		static void __fastcall HookedSimulationTicks(
			int ticksToAdvance, float* param_2);

		typedef void(__fastcall* SimulationTicks_t)(
			int ticksToAdvance, float* param_2);

		static inline SimulationTicks_t m_OriginalFunction{ nullptr };
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}