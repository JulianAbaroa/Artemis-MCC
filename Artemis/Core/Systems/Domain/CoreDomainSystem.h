#pragma once

#include <memory>

// Legacy.
//class EventRegistrySystem;
//class TimelineSystem;

// Map.
class MapSystem;
class MapTagGroupSystem;

// Navigation.
class SbspGeometryBuilder;
class SbspSeamLinker;
class NavigationSystem;

class ObjectTableSystem;
class PlayerTableSystem;
class InteractionTableSystem;

class ObjectGraphSystem;
class InteractableSystem;

struct CoreDomainSystem
{
	CoreDomainSystem();
	~CoreDomainSystem();

	//std::unique_ptr<EventRegistrySystem> EventRegistry;
	//std::unique_ptr<TimelineSystem> Timeline;

	std::unique_ptr<MapSystem> Map;
	std::unique_ptr<MapTagGroupSystem> MapTagGroup;

	std::unique_ptr<SbspGeometryBuilder> GeometryBuilder;
	std::unique_ptr<SbspSeamLinker> SeamLinker;
	std::unique_ptr<NavigationSystem> Navigation;

	std::unique_ptr<ObjectTableSystem> ObjectTable;
	std::unique_ptr<PlayerTableSystem> PlayerTable;
	std::unique_ptr<InteractionTableSystem> InteractionTable;

	std::unique_ptr<ObjectGraphSystem> ObjectGraph;
	std::unique_ptr<InteractableSystem> Interactable;
};