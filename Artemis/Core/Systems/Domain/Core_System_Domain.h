#pragma once

#include <memory>

// Legacy.
//class System_EventRegistry;
//class System_Timeline;

// Map.
class System_Map;
class System_MapTagGroup;

// Environment.
class System_CollGeometryBuilder;
class System_PhmoGeometryBuilder;
class System_ModeGeometryBuilder;
class System_ScnrZoneBuilder;
class System_BipdDataBuilder;
class System_Environment;

// Navigation.
class System_SbspGeometryBuilder;
class System_SbspSeamLinker;
class System_Navigation;

// Tables.
class System_ObjectTable;
class System_PlayerTable;
class System_InteractionTable;

class System_ObjectGraph;

// Interactables.
class System_VehiDataBuilder;
class System_Interactable;

struct Core_System_Domain
{
	Core_System_Domain();
	~Core_System_Domain();

	//std::unique_ptr<EventRegistrySystem> EventRegistry;
	//std::unique_ptr<TimelineSystem> Timeline;

	std::unique_ptr<System_Map> Map;
	std::unique_ptr<System_MapTagGroup> MapTagGroup;

	std::unique_ptr<System_CollGeometryBuilder> CollGeometryBuilder;
	std::unique_ptr<System_PhmoGeometryBuilder> PhmoGeometryBuilder;
	std::unique_ptr<System_ModeGeometryBuilder> ModeGeometryBuilder;
	std::unique_ptr<System_ScnrZoneBuilder> ScnrZoneBuilder;
	std::unique_ptr<System_BipdDataBuilder> BipdDataBuilder;
	std::unique_ptr<System_Environment> Environment;

	std::unique_ptr<System_SbspGeometryBuilder> SbspGeometryBuilder;
	std::unique_ptr<System_SbspSeamLinker> SbspSeamLinker;
	std::unique_ptr<System_Navigation> Navigation;

	std::unique_ptr<System_ObjectTable> ObjectTable;
	std::unique_ptr<System_PlayerTable> PlayerTable;
	std::unique_ptr<System_InteractionTable> InteractionTable;

	std::unique_ptr<System_ObjectGraph> ObjectGraph;

	std::unique_ptr<System_VehiDataBuilder> VehiDataBuilder;
	std::unique_ptr<System_Interactable> Interactable;
};