#include "pch.h"

// Header.
#include "Core_System_Domain.h"

// Dependecies.
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Core/Systems/Interface/Core_System_Interface.h"
#include "Core/Systems/Interface/Debug/System_Debug.h"

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

Core_System_Domain::Core_System_Domain() = default;
Core_System_Domain::~Core_System_Domain() = default;

void Core_System_Domain::Initialize(Core_State& state,
	Core_System_Infrastructure& systemInfrastructure, 
	Core_System_Interface& systemInterface)
{
	// Map.
	Map = std::make_unique<System_Map>(
		System_Map_Dependencies {
			.State_Map = *state.Domain->Map,
			.System_Debug = *systemInterface.Debug,
		}
	);

	MapTagGroup = std::make_unique<System_MapTagGroup>(
		System_MapTagGroup_Dependencies {
			.System_Map = *Map,
			.State_Map = *state.Domain->Map,
			.State_MapPhmo = *state.Domain->MapPhmo,
			.State_MapColl = *state.Domain->MapColl,
			.State_MapMode = *state.Domain->MapMode,
			.State_MapJmad = *state.Domain->MapJmad,
			.State_MapVehi = *state.Domain->MapVehi,
			.State_MapBloc = *state.Domain->MapBloc,
			.State_MapSbsp = *state.Domain->MapSbsp,
			.State_MapWeap = *state.Domain->MapWeap,
			.State_MapProj = *state.Domain->MapProj,
			.State_MapBipd = *state.Domain->MapBipd,
			.State_MapEqip = *state.Domain->MapEqip,
			.State_MapScen = *state.Domain->MapScen,
			.State_MapScnr = *state.Domain->MapScnr,
			.State_MapCtrl = *state.Domain->MapCtrl,
			.State_MapMach = *state.Domain->MapMach,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// Tables.
	ObjectTable = std::make_unique<System_ObjectTable>(
		System_ObjectTable_Dependencies {
			.System_Map = *Map,
			.System_MemoryReader = *systemInfrastructure.MemoryReader,
			.State_ObjectTable = *state.Domain->ObjectTable,
			.State_MapPhmo = *state.Domain->MapPhmo,
			.State_MapColl = *state.Domain->MapColl,
			.State_MapMode = *state.Domain->MapMode,
			.State_MapJmad = *state.Domain->MapJmad,
			.State_MapVehi = *state.Domain->MapVehi,
			.State_MapBloc = *state.Domain->MapBloc,
			.State_MapSbsp = *state.Domain->MapSbsp,
			.State_MapWeap = *state.Domain->MapWeap,
			.State_MapProj = *state.Domain->MapProj,
			.State_MapBipd = *state.Domain->MapBipd,
			.State_MapEqip = *state.Domain->MapEqip,
			.State_MapScen = *state.Domain->MapScen,
			.State_MapScnr = *state.Domain->MapScnr,
			.State_MapCtrl = *state.Domain->MapCtrl,
			.State_MapMach = *state.Domain->MapMach,
			.System_Debug = *systemInterface.Debug,
		}
	);

	PlayerTable = std::make_unique<System_PlayerTable>(
		System_PlayerTable_Dependencies {
			.State_PlayerTable = *state.Domain->PlayerTable,
			.System_MemoryReader = *systemInfrastructure.MemoryReader,
			.System_Debug = *systemInterface.Debug,
		}
	);

	InteractionTable = std::make_unique<System_InteractionTable>(
		System_InteractionTable_Dependencies {
			.State_InteractionTable = *state.Domain->InteractionTable,
			.System_MemoryReader = *systemInfrastructure.MemoryReader,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// Classifier.
	ObjectClassifier = std::make_unique<System_ObjectClassifier>(
		System_ObjectClassifier_Dependencies{
			.State_ObjectTable = *state.Domain->ObjectTable,
			.State_PlayerTable = *state.Domain->PlayerTable,
			.State_Classification = *state.Domain->Classification,
			.State_Navigation = *state.Domain->Navigation,
			.State_Environment = *state.Domain->Environment,
			.State_Interactable = *state.Domain->Interactable,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// Graphs.
	ObjectGraph = std::make_unique<System_ObjectGraph>(
		System_ObjectGraph_Dependencies {
			.State_ObjectTable = *state.Domain->ObjectTable,
			.State_ObjectGraph = *state.Domain->ObjectGraph,
			.System_Debug = *systemInterface.Debug,
		}
	);

	PlayerGraph = std::make_unique<System_PlayerGraph>(
		System_PlayerGraph_Dependencies {
			.State_ObjectTable = *state.Domain->ObjectTable,
			.State_PlayerTable = *state.Domain->PlayerTable,
			.State_Classification = *state.Domain->Classification,
			.State_ObjectGraph = *state.Domain->ObjectGraph,
			.State_PlayerGraph = *state.Domain->PlayerGraph,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// Navigation.
	SbspGeometryBuilder = std::make_unique<System_SbspGeometryBuilder>();
	SbspSeamLinker = std::make_unique<System_SbspSeamLinker>();
	ScenObstacleBuilder = std::make_unique<System_ScenObstacleBuilder>();
	BlocObstacleBuilder = std::make_unique<System_BlocObstacleBuilder>();
	BlocTeleporterBuilder = std::make_unique<System_BlocTeleporterBuilder>();
	MachDataBuilder = std::make_unique<System_MachDataBuilder>();

	Navigation = std::make_unique<System_Navigation>(
		System_Navigation_Dependencies {
			.State_Map = *state.Domain->Map,
			.State_MapSbsp = *state.Domain->MapSbsp,
			.State_MapScen = *state.Domain->MapScen,
			.State_MapBloc = *state.Domain->MapBloc,
			.State_MapMach = *state.Domain->MapMach,
			.State_ObjectTable = *state.Domain->ObjectTable,
			.State_Classification = *state.Domain->Classification,
			.State_Navigation = *state.Domain->Navigation,
			.System_SbspGeometryBuilder = *SbspGeometryBuilder,
			.System_SbspSeamLinker = *SbspSeamLinker,
			.System_ScenObstacleBuilder = *ScenObstacleBuilder,
			.System_BlocObstacleBuilder = *BlocObstacleBuilder,
			.System_BlocTeleporterBuilder = *BlocTeleporterBuilder,
			.System_MachDataBuilder = *MachDataBuilder,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// Environment.
	CollGeometryBuilder = std::make_unique<System_CollGeometryBuilder>();
	PhmoGeometryBuilder = std::make_unique<System_PhmoGeometryBuilder>();
	ModeGeometryBuilder = std::make_unique<System_ModeGeometryBuilder>();
	ScnrZoneBuilder = std::make_unique<System_ScnrZoneBuilder>();
	BipdDataBuilder = std::make_unique<System_BipdDataBuilder>();
	ScenZoneBuilder = std::make_unique<System_ScenZoneBuilder>();

	Environment = std::make_unique<System_Environment>(
		System_Environment_Dependencies {
			.State_Map = *state.Domain->Map,
			.State_MapColl = *state.Domain->MapColl,
			.State_MapPhmo = *state.Domain->MapPhmo,
			.State_MapMode = *state.Domain->MapMode,
			.State_MapScnr = *state.Domain->MapScnr,
			.State_MapBipd = *state.Domain->MapBipd,
			.State_MapScen = *state.Domain->MapScen,
			.State_ObjectTable = *state.Domain->ObjectTable,
			.State_Classification = *state.Domain->Classification,
			.State_Environment = *state.Domain->Environment,
			.System_CollGeometryBuilder = *CollGeometryBuilder,
			.System_PhmoGeometryBuilder = *PhmoGeometryBuilder,
			.System_ModeGeometryBuilder = *ModeGeometryBuilder,
			.System_ScnrZoneBuilder = *ScnrZoneBuilder,
			.System_BipdDataBuilder = *BipdDataBuilder,
			.System_ScenZoneBuilder = *ScenZoneBuilder,
			.System_Debug = *systemInterface.Debug,
		}
	);

	// Interactable.
	VehiDataBuilder = std::make_unique<System_VehiDataBuilder>();
	EqipDataBuilder = std::make_unique<System_EqipDataBuilder>();
	WeapDataBuilder = std::make_unique<System_WeapDataBuilder>();
	ProjDataBuilder = std::make_unique<System_ProjDataBuilder>();
	CtrlDataBuilder = std::make_unique<System_CtrlDataBuilder>();

	Interactable = std::make_unique<System_Interactable>(
		System_Interactable_Dependencies {
			.State_Map = *state.Domain->Map,
			.State_MapVehi = *state.Domain->MapVehi,
			.State_MapEqip = *state.Domain->MapEqip,
			.State_MapWeap = *state.Domain->MapWeap,
			.State_MapProj = *state.Domain->MapProj,
			.State_MapCtrl = *state.Domain->MapCtrl,
			.State_ObjectTable = *state.Domain->ObjectTable,
			.State_PlayerTable = *state.Domain->PlayerTable,
			.State_InteractionTable = *state.Domain->InteractionTable,
			.State_Classification = *state.Domain->Classification,
			.State_ObjectGraph = *state.Domain->ObjectGraph,
			.State_PlayerGraph = *state.Domain->PlayerGraph,
			.State_Environment = *state.Domain->Environment,
			.State_Interactable = *state.Domain->Interactable,
			.System_ObjectClassifier = *ObjectClassifier,
			.System_VehiDataBuilder = *VehiDataBuilder,
			.System_EqipDataBuilder = *EqipDataBuilder,
			.System_WeapDataBuilder = *WeapDataBuilder,
			.System_ProjDataBuilder = *ProjDataBuilder,
			.System_CtrlDataBuilder = *CtrlDataBuilder,
			.System_Debug = *systemInterface.Debug
		}
	);
}

void Core_System_Domain::Shutdown()
{
	Map.reset();
	MapTagGroup.reset();
	ObjectTable.reset();
	PlayerTable.reset();
	InteractionTable.reset();
	ObjectClassifier.reset();
	ObjectGraph.reset();
	PlayerGraph.reset();
	SbspGeometryBuilder.reset();
	SbspSeamLinker.reset();
	ScenObstacleBuilder.reset();
	BlocObstacleBuilder.reset();
	BlocTeleporterBuilder.reset();
	MachDataBuilder.reset();
	Navigation.reset();
	CollGeometryBuilder.reset();
	PhmoGeometryBuilder.reset();
	ModeGeometryBuilder.reset();
	ScnrZoneBuilder.reset();
	BipdDataBuilder.reset();
	ScenZoneBuilder.reset();
	Environment.reset();
	VehiDataBuilder.reset();
	EqipDataBuilder.reset();
	WeapDataBuilder.reset();
	ProjDataBuilder.reset();
	CtrlDataBuilder.reset();
	Interactable.reset();
}