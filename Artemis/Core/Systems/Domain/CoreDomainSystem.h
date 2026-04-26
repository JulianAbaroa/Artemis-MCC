#pragma once

#include <memory>

class EventRegistrySystem;
class TimelineSystem;

class MapSystem;
class MapTagGroupSystem;

//class MapPhmoSystem;
//class MapVehiSystem;
//class MapModeSystem;

class ObjectTableSystem;
class PlayerTableSystem;
class InteractionTableSystem;

class ObjectGraphSystem;
class InteractableSystem;

class PhmoSystem;
class VehiSystem;

struct CoreDomainSystem
{
	CoreDomainSystem();
	~CoreDomainSystem();

	std::unique_ptr<EventRegistrySystem> EventRegistry;
	std::unique_ptr<TimelineSystem> Timeline;

	std::unique_ptr<MapSystem> Map;
	std::unique_ptr<MapTagGroupSystem> MapTagGroup;

	//std::unique_ptr<MapPhmoSystem> MapPhmo;
	//std::unique_ptr<MapVehiSystem> MapVehi;
	//std::unique_ptr<MapModeSystem> MapMode;

	std::unique_ptr<ObjectTableSystem> ObjectTable;
	std::unique_ptr<PlayerTableSystem> PlayerTable;
	std::unique_ptr<InteractionTableSystem> InteractionTable;

	std::unique_ptr<ObjectGraphSystem> ObjectGraph;
	std::unique_ptr<InteractableSystem> Interactable;

	std::unique_ptr<PhmoSystem> Phmo;
	std::unique_ptr<VehiSystem> Vehi;
};