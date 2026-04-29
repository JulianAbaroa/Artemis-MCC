#include "pch.h"
#include "Core/States/Domain/Core_State_Domain.h"
//#include "Core/States/Domain/Legacy/State_Timeline.h"
//#include "Core/States/Domain/Legacy/State_EventRegistry.h"
#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Map/Phmo/State_MapPhmo.h"
#include "Core/States/Domain/Map/Coll/State_MapColl.h"
#include "Core/States/Domain/Map/Mode/State_MapMode.h"
#include "Core/States/Domain/Map/Vehi/State_MapVehi.h"
#include "Core/States/Domain/Map/Bloc/State_MapBloc.h"
#include "Core/States/Domain/Map/Sbsp/State_MapSbsp.h"
#include "Core/States/Domain/Map/Weap/State_MapWeap.h"
#include "Core/States/Domain/Map/Proj/State_MapProj.h"
#include "Core/States/Domain/Map/Bipd/State_MapBipd.h"
#include "Core/States/Domain/Map/Eqip/State_MapEqip.h"
#include "Core/States/Domain/Map/Scen/State_MapScen.h"
#include "Core/States/Domain/Map/Scnr/State_MapScnr.h"
#include "Core/States/Domain/Map/Jmad/State_MapJmad.h"
#include "Core/States/Domain/Map/Ctrl/State_MapCtrl.h"
#include "Core/States/Domain/Environment/State_Environment.h"
#include "Core/States/Domain/Navigation/State_Navigation.h"
#include "Core/States/Domain/Tables/State_ObjectTable.h"
#include "Core/States/Domain/Tables/State_PlayerTable.h"
#include "Core/States/Domain/Tables/State_InteractionTable.h"
#include "Core/States/Domain/Graph/State_ObjectGraph.h"
#include "Core/States/Domain/Interactable/State_Interactable.h"

Core_State_Domain::Core_State_Domain()
{
	//EventRegistry = std::make_unique<EventRegistryState>();
	//Timeline = std::make_unique<TimelineState>();

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

	Environment = std::make_unique<State_Environment>();

	Navigation = std::make_unique<State_Navigation>();

	ObjectTable = std::make_unique<State_ObjectTable>();
	PlayerTable = std::make_unique<State_PlayerTable>();
	InteractionTable = std::make_unique<State_InteractionTable>();

	ObjectGraph = std::make_unique<State_ObjectGraph>();
	Interactable = std::make_unique<State_Interactable>();
}

Core_State_Domain::~Core_State_Domain() = default;