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

struct Hook_SimulationTicks_Dependencies
{
	State_Telemetry& State_Telemetry;
	State_Lifecycle& State_Lifecycle;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
};

class Hook_SimulationTicks
{
public:
	Hook_SimulationTicks(Hook_SimulationTicks_Dependencies dependencies) :
		m_Deps(dependencies) {
	}
	~Hook_SimulationTicks() = default;

	void Install();
	void Uninstall();

	// --- Telemetry (Phase 1: measurement) ---
	static inline std::atomic<uint64_t> s_FrameCount{ 0 };     // calls with n>0... see note
	static inline std::atomic<uint64_t> s_CallCount{ 0 };      // all calls
	static inline std::atomic<uint64_t> s_TickTotal{ 0 };      // sum of ticksToAdvance
	static inline std::atomic<uint64_t> s_Hist0{ 0 };          // frames, 0 ticks
	static inline std::atomic<uint64_t> s_Hist1{ 0 };          // frames, 1 tick
	static inline std::atomic<uint64_t> s_Hist2{ 0 };          // frames, 2 ticks
	static inline std::atomic<uint64_t> s_Hist3Plus{ 0 };      // frames, 3+ ticks
	static inline std::atomic<uint64_t> s_DurationNsAccum{ 0 };// sum of original-call duration

private:
	static Hook_SimulationTicks* s_Instance;
	Hook_SimulationTicks_Dependencies m_Deps;

	static void __fastcall HookedSimulationTicks(
		int ticksToAdvance, float* outRemainingTime);

	typedef void(__fastcall* SimulationTicks_t)(
		int ticksToAdvance, float* outRemainingTime);

	static inline SimulationTicks_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};