#pragma once

#include <cstdint>
#include <atomic>
#include <chrono>

using SteadyClock = std::chrono::steady_clock;
using NanoSeconds = std::chrono::nanoseconds;

class State_Telemetry;
class State_Lifecycle;
class System_AOBScanner;
class System_Logs;

struct Hook_SimulationTicks_Deps
{
	State_Telemetry& State_Telemetry;
	State_Lifecycle& State_Lifecycle;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
};

class Hook_SimulationTicks
{
public:
	Hook_SimulationTicks(Hook_SimulationTicks_Deps deps) : m_Deps(deps) {}
	~Hook_SimulationTicks() = default;

	void Install();
	void Uninstall();

private:
	static Hook_SimulationTicks* s_Instance;
	Hook_SimulationTicks_Deps m_Deps;

	static void __fastcall HookedSimulationTicks(
		int ticksToAdvance, float* param_2);

	typedef void(__fastcall* SimulationTicks_t)(
		int ticksToAdvance, float* param_2);

	static inline SimulationTicks_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};