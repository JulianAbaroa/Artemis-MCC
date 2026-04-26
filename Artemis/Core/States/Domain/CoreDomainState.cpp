#include "pch.h"
#include "Core/States/Domain/CoreDomainState.h"
//#include "Core/States/Domain/Legacy/TimelineState.h"
//#include "Core/States/Domain/Legacy/EventRegistryState.h"
#include "Core/States/Domain/Map/MapState.h"
#include "Core/States/Domain/Map/Phmo/MapPhmoState.h"
#include "Core/States/Domain/Map/Coll/MapCollState.h"
#include "Core/States/Domain/Map/Mode/MapModeState.h"
#include "Core/States/Domain/Map/Vehi/MapVehiState.h"
#include "Core/States/Domain/Map/Bloc/MapBlocState.h"
#include "Core/States/Domain/Map/Sbsp/MapSbspState.h"
#include "Core/States/Domain/Map/Weap/MapWeapState.h"
#include "Core/States/Domain/Map/Proj/MapProjState.h"
#include "Core/States/Domain/Map/Bipd/MapBipdState.h"
#include "Core/States/Domain/Map/Eqip/MapEqipState.h"
#include "Core/States/Domain/Map/Scen/MapScenState.h"
#include "Core/States/Domain/Map/Scnr/MapScnrState.h"
#include "Core/States/Domain/Map/Jmad/MapJmadState.h"
#include "Core/States/Domain/Tables/ObjectTableState.h"
#include "Core/States/Domain/Tables/PlayerTableState.h"
#include "Core/States/Domain/Tables/InteractionTableState.h"
#include "Core/States/Domain/Graph/ObjectGraphState.h"
#include "Core/States/Domain/Interactable/InteractableState.h"
#include "Core/States/Domain/Tags/Phmo/PhmoState.h"
#include "Core/States/Domain/Tags/Vehi/VehiState.h"

CoreDomainState::CoreDomainState()
{
	//EventRegistry = std::make_unique<EventRegistryState>();
	//Timeline = std::make_unique<TimelineState>();

	Map = std::make_unique<MapState>();
	MapPhmo = std::make_unique<MapPhmoState>();
	MapVehi = std::make_unique<MapVehiState>();
	MapColl = std::make_unique<MapCollState>();
	MapMode = std::make_unique<MapModeState>();
	MapBloc = std::make_unique<MapBlocState>();
	MapSbsp = std::make_unique<MapSbspState>();
	MapWeap = std::make_unique<MapWeapState>();
	MapProj = std::make_unique<MapProjState>();
	MapBipd = std::make_unique<MapBipdState>();
	MapEqip = std::make_unique<MapEqipState>();
	MapScen = std::make_unique<MapScenState>();
	MapScnr = std::make_unique<MapScnrState>();
	MapJmad = std::make_unique<MapJmadState>();

	ObjectTable = std::make_unique<ObjectTableState>();
	PlayerTable = std::make_unique<PlayerTableState>();
	InteractionTable = std::make_unique<InteractionTableState>();

	ObjectGraph = std::make_unique<ObjectGraphState>();
	Interactable = std::make_unique<InteractableState>();

	Phmo = std::make_unique<PhmoState>();
	Vehi = std::make_unique<VehiState>();
}

CoreDomainState::~CoreDomainState() = default;