#pragma once

#include <memory>

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
class State_MapMach;

// Object.
class State_ObjectTable;

// Player.
class State_PlayerTable;

// Interaction.
class State_InteractionTable;

// Classification.
class State_Classification;

// Graph.
class State_ObjectGraph;
class State_PlayerGraph;

// Navigation
class State_Navigation;

// Environment.
class State_Environment;

// Interactable.
class State_Interactable;

// Main container for the application's domain states.
struct Core_State_Domain
{
	Core_State_Domain();
	~Core_State_Domain();

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
	std::unique_ptr<State_MapMach> MapMach;

	// Object.
	std::unique_ptr<State_ObjectTable> ObjectTable;

	// Player.
	std::unique_ptr<State_PlayerTable> PlayerTable;

	// Interaction.
	std::unique_ptr<State_InteractionTable> InteractionTable;

	// Classification.
	std::unique_ptr<State_Classification> Classification;

	// Graph.
	std::unique_ptr<State_ObjectGraph> ObjectGraph;
	std::unique_ptr<State_PlayerGraph> PlayerGraph;

	// Navigation.
	std::unique_ptr<State_Navigation> Navigation;

	// Environment.
	std::unique_ptr<State_Environment> Environment;

	// Interactable.
	std::unique_ptr<State_Interactable> Interactable;
};