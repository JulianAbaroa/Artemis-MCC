#include "pch.h"

#include "Hook_SimulationTicks.h"

#include "Core/Types/Other/Memory/AOB/Signatures.h"

#include "Core/States/Other/Telemetry/State_Telemetry.h"
#include "Core/States/Other/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/Other/Memory/AOB/System_AOBScanner.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

#include "External/minhook/include/MinHook.h"

void Hook_SimulationTicks::HookedSimulationTicks(
	int ticksToAdvance, float* outRemainingTime)
{
	auto start = SteadyClock::now();
	m_OriginalFunction(ticksToAdvance, outRemainingTime);
	auto end = SteadyClock::now();

	auto elapsed = end - start;
	uint64_t durationNS = std::chrono::duration_cast<NanoSeconds>(elapsed).count();

	s_Instance->m_Deps.State_Telemetry.RecordSimTick(
		ticksToAdvance, durationNS);

	if (ticksToAdvance > 0)
	{
		s_Instance->m_Deps.State_Lifecycle.SignalTick();
	}
}

Hook_SimulationTicks* Hook_SimulationTicks::s_Instance = nullptr;

void Hook_SimulationTicks::Install()
{
	if (m_IsHookInstalled.load()) return;
	s_Instance = this;

	void* functionAddress = (void*)s_Instance->m_Deps.
		System_AOBScanner.FindPattern(Signatures::SimulationTicks);

	if (!functionAddress)
	{
		s_Instance->m_Deps.System_Logs.Log("[SimulationTicks] ERROR:"
			" Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(),
		&this->HookedSimulationTicks,
		reinterpret_cast<LPVOID*>(&m_OriginalFunction)
	) != MH_OK)
	{
		s_Instance->m_Deps.System_Logs.Log("[SimulationTicks] ERROR:"
			" Failed to create the hook.");

		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		s_Instance->m_Deps.System_Logs.Log(" [SimulationTicks] ERROR:"
			" Failed to enable hook.");

		return;
	}

	m_IsHookInstalled.store(true);
	s_Instance->m_Deps.System_Logs.Log("[SimulationTicks] INFO:"
		" Hook installed.");
	return;
}

void Hook_SimulationTicks::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);

	s_Instance->m_Deps.System_Logs.Log("[SimulationTicks] INFO:"
		" Hook uninstalled.");

	s_Instance = nullptr;
}