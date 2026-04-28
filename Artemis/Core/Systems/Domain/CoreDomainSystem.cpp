#include "pch.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
//#include "Core/Systems/Domain/Legacy/EventRegistrySystem.h"
//#include "Core/Systems/Domain/Legacy/TimelineSystem.h"
#include "Core/Systems/Domain/Map/MapSystem.h"
#include "Core/Systems/Domain/Map/MapTagGroupSystem.h"
#include "Core/Systems/Domain/Map/Sbsp/SbspGeometryBuilder.h"
#include "Core/Systems/Domain/Map/Sbsp/SbspSeamLinker.h"
#include "Core/Systems/Domain/Navigation/NavigationSystem.h"
#include "Core/Systems/Domain/Tables/ObjectTableSystem.h"
#include "Core/Systems/Domain/Tables/PlayerTableSystem.h"
#include "Core/Systems/Domain/Tables/InteractionTableSystem.h"
#include "Core/Systems/Domain/Graph/ObjectGraphSystem.h"
#include "Core/Systems/Domain/Interactable/InteractableSystem.h"

CoreDomainSystem::CoreDomainSystem()
{
	//EventRegistry = std::make_unique<EventRegistrySystem>();
	//Timeline = std::make_unique<TimelineSystem>();

	Map = std::make_unique<MapSystem>();
	MapTagGroup = std::make_unique<MapTagGroupSystem>();

	GeometryBuilder = std::make_unique<SbspGeometryBuilder>();
	SeamLinker = std::make_unique<SbspSeamLinker>();
	Navigation = std::make_unique<NavigationSystem>();

	ObjectTable = std::make_unique<ObjectTableSystem>();
	PlayerTable = std::make_unique<PlayerTableSystem>();
	InteractionTable = std::make_unique<InteractionTableSystem>();

	ObjectGraph = std::make_unique<ObjectGraphSystem>();
	Interactable = std::make_unique<InteractableSystem>();
}

CoreDomainSystem::~CoreDomainSystem() = default;