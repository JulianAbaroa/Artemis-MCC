module;

#include "External/minhook/include/MinHook.h"

module Export.Tick.Hook;

import Platform.Memory.Type;
import Platform.Lifecycle.Type;
import Platform.Hook.Common;

namespace
{
	namespace Signature = Platform::Memory::Type::Signature;

	using Status = Platform::Lifecycle::Type::Status;
}

namespace Export::Tick::Hook
{
	auto SimulationTicksDetour::HookedSimulationTicks(int ticksToAdvance, float* param_2) -> void
	{
		s_Instance->m_LifecycleStore.SetStatus({ Status::Running });

		auto start = SteadyClock::now();
		m_OriginalFunction(ticksToAdvance, param_2);
		auto end = SteadyClock::now();

		auto elapsed = end - start;
		uint64_t durationNS = std::chrono::duration_cast<NanoSeconds>(elapsed).count();

		s_Instance->m_TelemetryStore.RecordSimTick(
			ticksToAdvance, durationNS);

		if (ticksToAdvance > 0)
		{
			s_Instance->m_LifecycleStore.SignalTick();
		}
	}

	SimulationTicksDetour* SimulationTicksDetour::s_Instance = nullptr;

	void SimulationTicksDetour::Install()
	{
		if (m_IsHookInstalled.load()) return;
		s_Instance = this;

		void* functionAddress = (void*)m_AOBService.FindPattern(Signature::SimulationTicks);
		m_FunctionAddress.store(functionAddress);

		if (!Platform::Hook::Common::InstallDetour(functionAddress,
			reinterpret_cast<void*>(&HookedSimulationTicks),
			reinterpret_cast<void**>(&m_OriginalFunction),
			"[SimulationTicksDetour]", m_LogsService))
		{
			return;
		}

		m_IsHookInstalled.store(true);
	}

	void SimulationTicksDetour::Uninstall()
	{
		if (!m_IsHookInstalled.load()) return;

		Platform::Hook::Common::UninstallDetour(m_FunctionAddress.load(),
			"[SimulationTicksDetour]", m_LogsService);

		m_IsHookInstalled.store(false);
		s_Instance = nullptr;
	}
}