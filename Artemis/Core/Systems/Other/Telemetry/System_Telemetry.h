#pragma once

#include <chrono>

using TimePoint = std::chrono::steady_clock::time_point;
using SteadyClock = std::chrono::steady_clock;
using Duration_Double = std::chrono::duration<double>;

class State_Telemetry;

struct Sys_Telemetry_Deps
{
	State_Telemetry& State_Telemetry;
};

class System_Telemetry
{
public:
	System_Telemetry(Sys_Telemetry_Deps deps) : m_Deps(deps) {}
	~System_Telemetry() = default;

	void Update();

private:
	Sys_Telemetry_Deps m_Deps;

	TimePoint m_LastUpdate = SteadyClock::now();
};