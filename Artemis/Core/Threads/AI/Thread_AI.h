#pragma once

#include <chrono>
#include <memory>

using SteadyClock = std::chrono::steady_clock;
using NanoSeconds = std::chrono::nanoseconds;

class State_MapReader;
class State_WorldBuilder;
class State_Telemetry;
class State_Lifecycle;
class System_TagGroupReader;
class System_WorldBuilder;
class System_StatsBuilder;
class System_VitalityBuilder;
class System_ObjectTable;
class System_PlayerTable;
class System_InteractionTable;
class System_Classifier;
class System_ObjectGraph;
class System_PlayerGraph;
class System_Collidables;
class System_Fixtures;
class System_Affordances;
class System_Vitality;
class System_Self;
class System_Tick;
class System_Logs;

struct Thread_AI_Deps
{
	State_MapReader& State_MapReader;
	State_WorldBuilder& State_WorldBuilder;
	State_Lifecycle& State_Lifecycle;
	State_Telemetry& State_Telemetry;
	System_TagGroupReader& System_TagGroupReader;
	System_WorldBuilder& System_WorldBuilder;
	System_StatsBuilder& System_StatsBuilder;
	System_VitalityBuilder& System_VitalityBuilder;
	System_ObjectTable& System_ObjectTable;
	System_PlayerTable& System_PlayerTable;
	System_InteractionTable& System_InteractionTable;
	System_Classifier& System_Classifier;
	System_ObjectGraph& System_ObjectGraph;
	System_PlayerGraph& System_PlayerGraph;
	System_Collidables& System_Collidables;
	System_Fixtures& System_Fixtures;
	System_Affordances& System_Affordances;
	System_Vitality& System_Vitality;
	System_Self& System_Self;
	System_Tick& System_Tick;
	System_Logs& System_Logs;
};

class Thread_AI
{
public:
	Thread_AI(Thread_AI_Deps deps) : m_Deps(deps) {}
	~Thread_AI() = default;

	void Run();

private:
	Thread_AI_Deps m_Deps;

	bool m_WasLoaded = false;
};