#pragma once

#include <memory>

// TODO: Rework these, not priority.
//class TimelineState;
//class EventRegistryState;

// Map.
class State_Map;
class State_MapPhmo;
class State_MapVehi;
class State_MapColl;
class State_MapMode;
class State_MapBloc;
class State_MapSbsp;
class State_MapWeap;
class State_MapProj;
class State_MapBipd;
class State_MapEqip;
class State_MapScen;
class State_MapScnr;
class State_MapJmad;
class State_MapCtrl;

// Environment.
class State_Environment;

// Navigation
class State_Navigation;

// Tables
class State_ObjectTable;
class State_PlayerTable;
class State_InteractionTable;

// AI Processed-data (TODO: ObservableSystem from which 
// the AI is going to receive the final polished data).
class State_ObjectGraph;
class State_Interactable;

// Main container for the application's domain states.
struct Core_State_Domain
{
	Core_State_Domain();
	~Core_State_Domain();

	//std::unique_ptr<State_Timeline> Timeline;
	//std::unique_ptr<State_EventRegistry> EventRegistry;

	// Map.
	std::unique_ptr<State_Map> Map;
	std::unique_ptr<State_MapPhmo> MapPhmo;
	std::unique_ptr<State_MapVehi> MapVehi;
	std::unique_ptr<State_MapColl> MapColl;
	std::unique_ptr<State_MapMode> MapMode;
	std::unique_ptr<State_MapBloc> MapBloc;
	std::unique_ptr<State_MapSbsp> MapSbsp;
	std::unique_ptr<State_MapWeap> MapWeap;
	std::unique_ptr<State_MapProj> MapProj;
	std::unique_ptr<State_MapBipd> MapBipd;
	std::unique_ptr<State_MapEqip> MapEqip;
	std::unique_ptr<State_MapScen> MapScen;
	std::unique_ptr<State_MapScnr> MapScnr;
	std::unique_ptr<State_MapJmad> MapJmad;
	std::unique_ptr<State_MapCtrl> MapCtrl;

	// Environment.
	std::unique_ptr<State_Environment> Environment;

	// Navigation.
	std::unique_ptr<State_Navigation> Navigation;

	// Tables.
	std::unique_ptr<State_ObjectTable> ObjectTable;
	std::unique_ptr<State_PlayerTable> PlayerTable;
	std::unique_ptr<State_InteractionTable> InteractionTable;

	std::unique_ptr<State_ObjectGraph> ObjectGraph;

	// Interactable.
	std::unique_ptr<State_Interactable> Interactable;
};