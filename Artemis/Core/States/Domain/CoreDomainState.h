#pragma once

#include <memory>

// TODO: Rework these, not priority.
//class TimelineState;
//class EventRegistryState;

// Maps.
class MapState;
class MapPhmoState;
class MapVehiState;
class MapCollState;
class MapModeState;
class MapBlocState;
class MapSbspState;
class MapWeapState;
class MapProjState;
class MapBipdState;
class MapEqipState;
class MapScenState;
class MapScnrState;
class MapJmadState;
class MapCtrlState;

// Tables
class ObjectTableState;
class PlayerTableState;
class InteractionTableState;

// AI Processed-data (TODO: ObservableSystem from which 
// the AI is going to receive the final polished data).
class ObjectGraphState;
class InteractableState;

// Main container for the application's domain states.
struct CoreDomainState
{
	CoreDomainState();
	~CoreDomainState();

	//std::unique_ptr<TimelineState> Timeline;
	//std::unique_ptr<EventRegistryState> EventRegistry;

	std::unique_ptr<MapState> Map;
	std::unique_ptr<MapPhmoState> MapPhmo;
	std::unique_ptr<MapVehiState> MapVehi;
	std::unique_ptr<MapCollState> MapColl;
	std::unique_ptr<MapModeState> MapMode;
	std::unique_ptr<MapBlocState> MapBloc;
	std::unique_ptr<MapSbspState> MapSbsp;
	std::unique_ptr<MapWeapState> MapWeap;
	std::unique_ptr<MapProjState> MapProj;
	std::unique_ptr<MapBipdState> MapBipd;
	std::unique_ptr<MapEqipState> MapEqip;
	std::unique_ptr<MapScenState> MapScen;
	std::unique_ptr<MapScnrState> MapScnr;
	std::unique_ptr<MapJmadState> MapJmad;
	std::unique_ptr<MapCtrlState> MapCtrl;

	std::unique_ptr<ObjectTableState> ObjectTable;
	std::unique_ptr<PlayerTableState> PlayerTable;
	std::unique_ptr<InteractionTableState> InteractionTable;

	std::unique_ptr<ObjectGraphState> ObjectGraph;
	std::unique_ptr<InteractableState> Interactable;
};