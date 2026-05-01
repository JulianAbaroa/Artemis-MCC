#include "pch.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
//#include "Core/Systems/Domain/Legacy/EventRegistrySystem.h"
//#include "Core/Systems/Domain/Legacy/TimelineSystem.h"
#include "Core/Systems/Domain/Map/System_Map.h"
#include "Core/Systems/Domain/Map/System_MapTagGroup.h"
#include "Core/Systems/Domain/Environment/Coll/System_CollGeometryBuilder.h"
#include "Core/Systems/Domain/Environment/Phmo/System_PhmoGeometryBuilder.h"
#include "Core/Systems/Domain/Environment/Mode/System_ModeGeometryBuilder.h"
#include "Core/Systems/Domain/Environment/Scnr/System_ScnrZoneBuilder.h"
#include "Core/Systems/Domain/Environment/Bipd/System_BipdDataBuilder.h"
#include "Core/Systems/Domain/Environment/Scen/System_ScenZoneBuilder.h"
#include "Core/Systems/Domain/Environment/System_Environment.h"
#include "Core/Systems/Domain/Navigation/Sbsp/System_SbspGeometryBuilder.h"
#include "Core/Systems/Domain/Navigation/Sbsp/System_SbspSeamLinker.h"
#include "Core/Systems/Domain/Navigation/Scen/System_ScenObstacleBuilder.h"
#include "Core/Systems/Domain/Navigation/Bloc/System_BlocObstacleBuilder.h"
#include "Core/Systems/Domain/Navigation/Bloc/System_BlocTeleporterBuilder.h"
#include "Core/Systems/Domain/Navigation/Mach/System_MachDataBuilder.h"
#include "Core/Systems/Domain/Navigation/System_Navigation.h"
#include "Core/Systems/Domain/Tables/System_ObjectTable.h"
#include "Core/Systems/Domain/Tables/System_PlayerTable.h"
#include "Core/Systems/Domain/Tables/System_InteractionTable.h"
#include "Core/Systems/Domain/Graph/System_ObjectGraph.h"
#include "Core/Systems/Domain/Interactable/Vehi/System_VehiDataBuilder.h"
#include "Core/Systems/Domain/Interactable/Eqip/System_EqipDataBuilder.h"
#include "Core/Systems/Domain/Interactable/Weap/System_WeapDataBuilder.h"
#include "Core/Systems/Domain/Interactable/Proj/System_ProjDataBuilder.h"
#include "Core/Systems/Domain/Interactable/Ctrl/System_CtrlDataBuilder.h"
#include "Core/Systems/Domain/Interactable/System_Interactable.h"

Core_System_Domain::Core_System_Domain()
{
	//EventRegistry = std::make_unique<EventRegistrySystem>();
	//Timeline = std::make_unique<TimelineSystem>();

	Map = std::make_unique<System_Map>();
	MapTagGroup = std::make_unique<System_MapTagGroup>();

	CollGeometryBuilder = std::make_unique<System_CollGeometryBuilder>();
	PhmoGeometryBuilder = std::make_unique<System_PhmoGeometryBuilder>();
	ModeGeometryBuilder = std::make_unique<System_ModeGeometryBuilder>();
	ScnrZoneBuilder = std::make_unique<System_ScnrZoneBuilder>();
	BipdDataBuilder = std::make_unique<System_BipdDataBuilder>();
	ScenZoneBuilder = std::make_unique<System_ScenZoneBuilder>();
	Environment = std::make_unique<System_Environment>();

	SbspGeometryBuilder = std::make_unique<System_SbspGeometryBuilder>();
	SbspSeamLinker = std::make_unique<System_SbspSeamLinker>();
	ScenObstacleBuilder = std::make_unique<System_ScenObstacleBuilder>();
	BlocObstacleBuilder = std::make_unique<System_BlocObstacleBuilder>();
	BlocTeleporterBuilder = std::make_unique<System_BlocTeleporterBuilder>();
	MachDataBuilder = std::make_unique<System_MachDataBuilder>();
	Navigation = std::make_unique<System_Navigation>();

	ObjectTable = std::make_unique<System_ObjectTable>();
	PlayerTable = std::make_unique<System_PlayerTable>();
	InteractionTable = std::make_unique<System_InteractionTable>();

	ObjectGraph = std::make_unique<System_ObjectGraph>();

	VehiDataBuilder = std::make_unique<System_VehiDataBuilder>();
	EqipDataBuilder = std::make_unique<System_EqipDataBuilder>();
	WeapDataBuilder = std::make_unique<System_WeapDataBuilder>();
	ProjDataBuilder = std::make_unique<System_ProjDataBuilder>();
	CtrlDataBuilder = std::make_unique<System_CtrlDataBuilder>();
	Interactable = std::make_unique<System_Interactable>();
}

Core_System_Domain::~Core_System_Domain() = default;