#pragma once

#include <cstdint>
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

	/**
	* @brief Entry and main point of the AI threa. Runs the entire
	* lifetime of the mod, driving Artemis through a state machine
	* keyed on the current lifecycle Status.
	*/
	void Run();

private:
	Thread_AI_Deps m_Deps;

	bool m_WasLoaded = false;

	uint64_t m_Last = 0;
	uint64_t m_Dropped = 0;

	/**
	* @brief Reads and builds the static map resources (i.e. tags),
	* from the loaded .map file.
	* @note Expensive (~1), called once per map load.
	*/
	void LoadResources();

	/**
	* @brief Runs the full per-tick transformation pipeline, advancing
	* the game state through the layered architecture. Each layer 
	* consumes the output of the previous one, so the call order
	* is significant.
	* @note Called once per signaled tick.
	*/
	void ExecuteTick();

	/**
	* @brief Determines if its safe to continue the resource loading or
	* the tick execution, by the current Artemis status.
	* @return true if Artemis is still running and if Blam it's not
	* tearing down.
	*/
	bool IsStable();

	/**  
	* @brief Clears the AI thread's per-session tracking state to its
	* initial values.
	*/
	void Reset();
};