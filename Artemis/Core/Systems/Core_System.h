#pragma once

#include <memory>

class Core_State;

// Related to .map files.
class System_MapReader;
class System_TagGroupReader;
class System_MeshReader;

// Related to game engine tables.
class System_ObjectTable; class System_PlayerTable;
class System_InteractionTable;

// Related to data specialization.
class System_ObjectGraph; class System_PlayerGraph;
class System_Classifier;

// Related to navigation domain.
class System_Navigation; class System_SbspBuilder; 
class System_SbspSeamLinker; class System_ScnrBuilder;

// Related to environment domain.
class System_Environment; class System_CollBuilder; 
class System_ModeBuilder; class System_PhmoBuilder;

// Related to interactable domain.
class System_Interactable; class System_ProjBuilder;
class System_VehiBuilder; class System_WeapBuilder;

// Related to game engine memory (RAM).
class System_AOBScanner; class System_MemoryReader;
class System_MemoryScanner;

// Other.
class System_Input; class System_Lifecycle; class System_Render;
class System_Settings; class System_Logs;

class Core_System
{
public:
	Core_System();
	~Core_System();

	void Initialize(Core_State& state);
	void Deinitialize();

	std::unique_ptr<System_MapReader> MapReader;
	std::unique_ptr<System_TagGroupReader> TagGroupReader;
	std::unique_ptr<System_MeshReader> MeshReader;

	std::unique_ptr<System_ObjectTable> ObjectTable;
	std::unique_ptr<System_PlayerTable> PlayerTable;
	std::unique_ptr<System_InteractionTable> InteractionTable;

	std::unique_ptr<System_ObjectGraph> ObjectGraph;
	std::unique_ptr<System_PlayerGraph> PlayerGraph;
	std::unique_ptr<System_Classifier> Classifier;

	std::unique_ptr<System_Navigation> Navigation;
	std::unique_ptr<System_SbspBuilder> SbspBuilder;
	std::unique_ptr<System_SbspSeamLinker> SbspSeamLinker;
	std::unique_ptr<System_ScnrBuilder> ScnrBuilder;

	std::unique_ptr<System_Environment> Environment;
	std::unique_ptr<System_CollBuilder> CollBuilder;
	std::unique_ptr<System_ModeBuilder> ModeBuilder;
	std::unique_ptr<System_PhmoBuilder> PhmoBuilder;

	std::unique_ptr<System_Interactable> Interactable;
	std::unique_ptr<System_ProjBuilder> ProjBuilder;
	std::unique_ptr<System_VehiBuilder> VehiBuilder;
	std::unique_ptr<System_WeapBuilder> WeapBuilder;

	std::unique_ptr<System_AOBScanner> AOBScanner;
	std::unique_ptr<System_MemoryReader> MemoryReader;
	std::unique_ptr<System_MemoryScanner> MemoryScanner;

	std::unique_ptr<System_Input> Input;
	std::unique_ptr<System_Lifecycle> Lifecycle;
	std::unique_ptr<System_Render> Render;
	std::unique_ptr<System_Settings> Settings;
	std::unique_ptr<System_Logs> Logs;

private:
	void InitMapReaders(Core_State& state);
	void DeinitMapReaders();

	void InitTables(Core_State& state);
	void DeinitTables();

	void InitFiltered(Core_State& state);
	void DeinitFiltered();

	void InitNavigation(Core_State& state);
	void DeinitNavigation();

	void InitEnvironment(Core_State& state);
	void DeinitEnvironment();

	void InitInteractable(Core_State& state);
	void DeinitInteractable();

	void InitMemory(Core_State& state);
	void DeinitMemory();
};