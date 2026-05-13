#include "pch.h"

// Header.
#include "Core_State_Domain.h"

// Map.
#include "Map/State_Map.h"
#include "Map/Phmo/State_MapPhmo.h"
#include "Map/Coll/State_MapColl.h"
#include "Map/Mode/State_MapMode.h"
#include "Map/Vehi/State_MapVehi.h"
#include "Map/Bloc/State_MapBloc.h"
#include "Map/Sbsp/State_MapSbsp.h"
#include "Map/Weap/State_MapWeap.h"
#include "Map/Proj/State_MapProj.h"
#include "Map/Bipd/State_MapBipd.h"
#include "Map/Eqip/State_MapEqip.h"
#include "Map/Scen/State_MapScen.h"
#include "Map/Scnr/State_MapScnr.h"
#include "Map/Jmad/State_MapJmad.h"
#include "Map/Ctrl/State_MapCtrl.h"
#include "Map/Mach/State_MapMach.h"

// Object.
#include "Object/State_ObjectTable.h"

// Player.
#include "Player/State_PlayerTable.h"

// Interaction.
#include "Interaction/State_InteractionTable.h"

// Classification.
#include "Classification/State_Classification.h"

// Graph.
#include "Graph/State_ObjectGraph.h"
#include "Graph/State_PlayerGraph.h"

// Navigation.
#include "Navigation/State_Navigation.h"

// Environment.
#include "Environment/State_Environment.h"

// Interactable.
#include "Interactable/State_Interactable.h"

Core_State_Domain::Core_State_Domain()
{
	// Map.
	Map = std::make_unique<State_Map>();
	MapPhmo = std::make_unique<State_MapPhmo>();
	MapVehi = std::make_unique<State_MapVehi>();
	MapColl = std::make_unique<State_MapColl>();
	MapMode = std::make_unique<State_MapMode>();
	MapBloc = std::make_unique<State_MapBloc>();
	MapSbsp = std::make_unique<State_MapSbsp>();
	MapWeap = std::make_unique<State_MapWeap>();
	MapProj = std::make_unique<State_MapProj>();
	MapBipd = std::make_unique<State_MapBipd>();
	MapEqip = std::make_unique<State_MapEqip>();
	MapScen = std::make_unique<State_MapScen>();
	MapScnr = std::make_unique<State_MapScnr>();
	MapJmad = std::make_unique<State_MapJmad>();
	MapCtrl = std::make_unique<State_MapCtrl>();
	MapMach = std::make_unique<State_MapMach>();

	// Object.
	ObjectTable = std::make_unique<State_ObjectTable>();

	// Player.
	PlayerTable = std::make_unique<State_PlayerTable>();

	// Interaction.
	InteractionTable = std::make_unique<State_InteractionTable>();

	// Classification.
	Classification = std::make_unique<State_Classification>();
	
	// Graph.
	ObjectGraph = std::make_unique<State_ObjectGraph>();
	PlayerGraph = std::make_unique<State_PlayerGraph>();

	// Navigation.
	Navigation = std::make_unique<State_Navigation>();

	// Environment.
	Environment = std::make_unique<State_Environment>();

	// Interactable.
	Interactable = std::make_unique<State_Interactable>();
}

Core_State_Domain::~Core_State_Domain() = default;