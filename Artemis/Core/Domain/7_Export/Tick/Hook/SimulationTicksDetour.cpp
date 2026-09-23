module;

#include "External/minhook/include/MinHook.h"

module Export.Tick.Hook;

import Platform.Memory.Type;
import Platform.Lifecycle.Type;

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

	auto SimulationTicksDetour::Install() -> void
	{
		if (m_IsHookInstalled.load()) return;
		s_Instance = this;

		void* functionAddress = (void*)s_Instance->m_AOBService.FindPattern(
			Signature::SimulationTicks);

		if (!functionAddress)
		{
			s_Instance->m_LogsService.Message("[SimulationTicksDetour] ERROR:"
				" Failed to obtain the function address.");
			return;
		}

		m_FunctionAddress.store(functionAddress);
		if (MH_CreateHook(m_FunctionAddress.load(),
			&this->HookedSimulationTicks,
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
		{
			s_Instance->m_LogsService.Message("[SimulationTicksDetour] ERROR:"
				" Failed to create the hook.");

			return;
		}
		if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
		{
			s_Instance->m_LogsService.Message(" [SimulationTicksDetour] ERROR:"
				" Failed to enable hook.");

			return;
		}

		m_IsHookInstalled.store(true);
		s_Instance->m_LogsService.Message("[SimulationTicksDetour] INFO:"
			" Hook installed.");
		return;
	}

	auto SimulationTicksDetour::Uninstall() -> void
	{
		if (!m_IsHookInstalled.load()) return;

		MH_DisableHook(m_FunctionAddress.load());
		MH_RemoveHook(m_FunctionAddress.load());

		m_IsHookInstalled.store(false);

		s_Instance->m_LogsService.Message("[SimulationTicksDetour] INFO:"
			" Hook uninstalled.");

		s_Instance = nullptr;
	}
}