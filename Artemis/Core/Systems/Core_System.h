#pragma once

#include <memory>

class Core_State;

// Sources
class System_MapReader;
class System_TagGroupReader;
class System_GeometryReader;
class System_ObjectTable; class System_PlayerTable;
class System_InteractionTable;

// Structures.
class System_ObjectGraph; class System_PlayerGraph;
class System_Classifier;

// Environment.
class System_StatsBuilder; class System_WeapBuilder; 
class System_VehiBuilder; class System_ProjBuilder;
class System_WorldBuilder; class System_Collidables;
class System_CollBuilder; class System_ModeBuilder;
class System_PhmoBuilder; class System_SbspBuilder;
class System_SbspSeamLinker; class System_VitalityBuilder;
class System_Vitality;
class System_Self; class System_Fixtures; 
class System_Affordances;

// Tick.
class System_Tick;

// Other.
class System_AOBScanner; class System_MemoryReader;
class System_MemoryScanner; class System_Input; 
class System_Telemetry; class System_Lifecycle; 
class System_Render; class System_MapRenderer;
class System_Settings; class System_Logs;

class Core_System
{
public:
	Core_System();
	~Core_System();

	void Initialize(Core_State& state);
	void Deinitialize();

	// --- Layer 0: Sources ---
	std::unique_ptr<System_MapReader> MapReader;
	std::unique_ptr<System_TagGroupReader> TagGroupReader;
	std::unique_ptr<System_GeometryReader> GeometryReader;
	std::unique_ptr<System_ObjectTable> ObjectTable;
	std::unique_ptr<System_PlayerTable> PlayerTable;
	std::unique_ptr<System_InteractionTable> InteractionTable;
	std::unique_ptr<System_StatsBuilder> StatsBuilder;
	std::unique_ptr<System_WeapBuilder> WeapBuilder;
	std::unique_ptr<System_VehiBuilder> VehiBuilder;
	std::unique_ptr<System_ProjBuilder> ProjBuilder;
	std::unique_ptr<System_WorldBuilder> WorldBuilder;
	std::unique_ptr<System_CollBuilder> CollBuilder;
	std::unique_ptr<System_ModeBuilder> ModeBuilder;
	std::unique_ptr<System_PhmoBuilder> PhmoBuilder;
	std::unique_ptr<System_SbspBuilder> SbspBuilder;
	std::unique_ptr<System_SbspSeamLinker> SbspSeamLinker;
	std::unique_ptr<System_VitalityBuilder> VitalityBuilder;

	// --- Layer 1: Structure ---
	std::unique_ptr<System_ObjectGraph> ObjectGraph;
	std::unique_ptr<System_PlayerGraph> PlayerGraph;
	std::unique_ptr<System_Classifier> Classifier;

	// --- Layer 2: Environment ---
	std::unique_ptr<System_Collidables> Collidables;
	std::unique_ptr<System_Vitality> Vitality;
	std::unique_ptr<System_Fixtures> Fixtures;

	// --- Layer 3: Egocentric ---
	std::unique_ptr<System_Self> Self;
	std::unique_ptr<System_Affordances> Affordances;

	// Tick.
	std::unique_ptr<System_Tick> Tick;

	// Other.
	std::unique_ptr<System_Input> Input;
	std::unique_ptr<System_Lifecycle> Lifecycle;
	std::unique_ptr<System_Logs> Logs;
	std::unique_ptr<System_AOBScanner> AOBScanner;
	std::unique_ptr<System_MemoryReader> MemoryReader;
	std::unique_ptr<System_MemoryScanner> MemoryScanner;
	std::unique_ptr<System_Render> Render;
	std::unique_ptr<System_MapRenderer> MapRenderer;
	std::unique_ptr<System_Settings> Settings;
	std::unique_ptr<System_Telemetry> Telemetry;

private:
	void InitSources(Core_State& state);
	void DeinitSources();

	void InitStructure(Core_State& state);
	void DeinitStructure();

	void InitEnvironment(Core_State& state);
	void DeinitEnvironment();

	void InitEgocentric(Core_State& state);
	void DeinitEgocentric();
};