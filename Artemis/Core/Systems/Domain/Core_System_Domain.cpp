#include "pch.h"

// Header.
#include "Core_System_Domain.h"

// Map.
#include "Map/System_Map.h"
#include "Map/System_MapTagGroup.h"

// Object.
#include "Object/System_ObjectTable.h"

// Player.
#include "Player/System_PlayerTable.h"

// Interaction.
#include "Interaction/System_InteractionTable.h"

// Classification.
#include "Classification/System_ObjectClassifier.h"

// Graph.
#include "Graph/System_ObjectGraph.h"
#include "Graph/System_PlayerGraph.h"

// Navigation.
#include "Navigation/Sbsp/System_SbspGeometryBuilder.h"
#include "Navigation/Sbsp/System_SbspSeamLinker.h"
#include "Navigation/Scen/System_ScenObstacleBuilder.h"
#include "Navigation/Bloc/System_BlocObstacleBuilder.h"
#include "Navigation/Bloc/System_BlocTeleporterBuilder.h"
#include "Navigation/Mach/System_MachDataBuilder.h"
#include "Navigation/System_Navigation.h"

// Environment.
#include "Environment/Coll/System_CollGeometryBuilder.h"
#include "Environment/Phmo/System_PhmoGeometryBuilder.h"
#include "Environment/Mode/System_ModeGeometryBuilder.h"
#include "Environment/Scnr/System_ScnrZoneBuilder.h"
#include "Environment/Bipd/System_BipdDataBuilder.h"
#include "Environment/Scen/System_ScenZoneBuilder.h"
#include "Environment/System_Environment.h"

// Interactable.
#include "Interactable/Vehi/System_VehiDataBuilder.h"
#include "Interactable/Eqip/System_EqipDataBuilder.h"
#include "Interactable/Weap/System_WeapDataBuilder.h"
#include "Interactable/Proj/System_ProjDataBuilder.h"
#include "Interactable/Ctrl/System_CtrlDataBuilder.h"
#include "Interactable/System_Interactable.h"

Core_System_Domain::Core_System_Domain()
{
	// Map.
	Map = std::make_unique<System_Map>();
	MapTagGroup = std::make_unique<System_MapTagGroup>();

	// Object.
	ObjectTable = std::make_unique<System_ObjectTable>();

	// Player.
	PlayerTable = std::make_unique<System_PlayerTable>();

	// Interaction.
	InteractionTable = std::make_unique<System_InteractionTable>();

	// Classification.
	ObjectClassifier = std::make_unique<System_ObjectClassifier>();

	// Graph.
	ObjectGraph = std::make_unique<System_ObjectGraph>();
	PlayerGraph = std::make_unique<System_PlayerGraph>();

	// Navigation.
	SbspGeometryBuilder = std::make_unique<System_SbspGeometryBuilder>();
	SbspSeamLinker = std::make_unique<System_SbspSeamLinker>();
	ScenObstacleBuilder = std::make_unique<System_ScenObstacleBuilder>();
	BlocObstacleBuilder = std::make_unique<System_BlocObstacleBuilder>();
	BlocTeleporterBuilder = std::make_unique<System_BlocTeleporterBuilder>();
	MachDataBuilder = std::make_unique<System_MachDataBuilder>();
	Navigation = std::make_unique<System_Navigation>();

	// Environment.
	CollGeometryBuilder = std::make_unique<System_CollGeometryBuilder>();
	PhmoGeometryBuilder = std::make_unique<System_PhmoGeometryBuilder>();
	ModeGeometryBuilder = std::make_unique<System_ModeGeometryBuilder>();
	ScnrZoneBuilder = std::make_unique<System_ScnrZoneBuilder>();
	BipdDataBuilder = std::make_unique<System_BipdDataBuilder>();
	ScenZoneBuilder = std::make_unique<System_ScenZoneBuilder>();
	Environment = std::make_unique<System_Environment>();

	// Interactable.
	VehiDataBuilder = std::make_unique<System_VehiDataBuilder>();
	EqipDataBuilder = std::make_unique<System_EqipDataBuilder>();
	WeapDataBuilder = std::make_unique<System_WeapDataBuilder>();
	ProjDataBuilder = std::make_unique<System_ProjDataBuilder>();
	CtrlDataBuilder = std::make_unique<System_CtrlDataBuilder>();
	Interactable = std::make_unique<System_Interactable>();
}

Core_System_Domain::~Core_System_Domain() = default;