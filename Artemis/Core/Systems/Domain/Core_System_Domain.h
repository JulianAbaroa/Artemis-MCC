#pragma once

#include <memory>

// Dependecies.
struct Core_State;
struct Core_System_Infrastructure;
struct Core_System_Interface;

// Map.
class System_Map;
class System_MapTagGroup;

// Object.
class System_ObjectTable;

// Player.
class System_PlayerTable;

// Interaction.
class System_InteractionTable;

// Classification.
class System_ObjectClassifier;

// Graph.
class System_ObjectGraph;
class System_PlayerGraph;

// Navigation.
class System_SbspGeometryBuilder;
class System_SbspSeamLinker;
class System_ScenObstacleBuilder;
class System_BlocObstacleBuilder;
class System_BlocTeleporterBuilder;
class System_MachDataBuilder;
class System_Navigation;

// Environment.
class System_CollGeometryBuilder;
class System_PhmoGeometryBuilder;
class System_ModeGeometryBuilder;
class System_ScnrZoneBuilder;
class System_BipdDataBuilder;
class System_ScenZoneBuilder;
class System_Environment;

// Interactable.
class System_VehiDataBuilder;
class System_EqipDataBuilder;
class System_WeapDataBuilder;
class System_ProjDataBuilder;
class System_CtrlDataBuilder;
class System_Interactable;

struct Core_System_Domain
{
	Core_System_Domain();
	~Core_System_Domain();

	void Initialize(Core_State& state, 
		Core_System_Infrastructure& systemInfrastructure,
		Core_System_Interface& systemInterface);
	void Shutdown();

	// Map.
	std::unique_ptr<System_Map> Map;
	std::unique_ptr<System_MapTagGroup> MapTagGroup;

	// Object.
	std::unique_ptr<System_ObjectTable> ObjectTable;

	// Player.
	std::unique_ptr<System_PlayerTable> PlayerTable;

	// Interaction.
	std::unique_ptr<System_InteractionTable> InteractionTable;

	// Classification.
	std::unique_ptr<System_ObjectClassifier> ObjectClassifier;

	// Graph.
	std::unique_ptr<System_ObjectGraph> ObjectGraph;
	std::unique_ptr<System_PlayerGraph> PlayerGraph;

	// Navigation.
	std::unique_ptr<System_SbspGeometryBuilder> SbspGeometryBuilder;
	std::unique_ptr<System_SbspSeamLinker> SbspSeamLinker;
	std::unique_ptr<System_ScenObstacleBuilder> ScenObstacleBuilder;
	std::unique_ptr<System_BlocObstacleBuilder> BlocObstacleBuilder;
	std::unique_ptr<System_BlocTeleporterBuilder> BlocTeleporterBuilder;
	std::unique_ptr<System_MachDataBuilder> MachDataBuilder;
	std::unique_ptr<System_Navigation> Navigation;

	// Environment.
	std::unique_ptr<System_CollGeometryBuilder> CollGeometryBuilder;
	std::unique_ptr<System_PhmoGeometryBuilder> PhmoGeometryBuilder;
	std::unique_ptr<System_ModeGeometryBuilder> ModeGeometryBuilder;
	std::unique_ptr<System_ScnrZoneBuilder> ScnrZoneBuilder;
	std::unique_ptr<System_BipdDataBuilder> BipdDataBuilder;
	std::unique_ptr<System_ScenZoneBuilder> ScenZoneBuilder;
	std::unique_ptr<System_Environment> Environment;

	// Interactable.
	std::unique_ptr<System_VehiDataBuilder> VehiDataBuilder;
	std::unique_ptr<System_EqipDataBuilder> EqipDataBuilder;
	std::unique_ptr<System_WeapDataBuilder> WeapDataBuilder;
	std::unique_ptr<System_ProjDataBuilder> ProjDataBuilder;
	std::unique_ptr<System_CtrlDataBuilder> CtrlDataBuilder;
	std::unique_ptr<System_Interactable> Interactable;
};