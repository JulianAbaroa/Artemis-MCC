#include "pch.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Legacy/EventRegistrySystem.h"
#include "Core/Systems/Domain/Legacy/TimelineSystem.h"

#include "Core/Systems/Domain/Map/MapSystem.h"
#include "Core/Systems/Domain/Map/MapTagGroupSystem.h"

//#include "Core/Systems/Domain/Map/MapPhmoSystem.h"
//#include "Core/Systems/Domain/Map/MapVehiSystem.h"
//#include "Core/Systems/Domain/Map/MapModeSystem.h"

#include "Core/Systems/Domain/Tables/ObjectTableSystem.h"
#include "Core/Systems/Domain/Tables/PlayerTableSystem.h"
#include "Core/Systems/Domain/Tables/InteractionTableSystem.h"
#include "Core/Systems/Domain/Graph/ObjectGraphSystem.h"
#include "Core/Systems/Domain/Interactable/InteractableSystem.h"
#include "Core/Systems/Domain/Tags/Phmo/PhmoSystem.h"
#include "Core/Systems/Domain/Tags/Vehi/VehiSystem.h"

CoreDomainSystem::CoreDomainSystem()
{
	EventRegistry = std::make_unique<EventRegistrySystem>();
	Timeline = std::make_unique<TimelineSystem>();

	Map = std::make_unique<MapSystem>();
	MapTagGroup = std::make_unique<MapTagGroupSystem>();

	//MapPhmo = std::make_unique<MapPhmoSystem>();
	//MapVehi = std::make_unique<MapVehiSystem>();
	//MapMode = std::make_unique<MapModeSystem>();

	ObjectTable = std::make_unique<ObjectTableSystem>();
	PlayerTable = std::make_unique<PlayerTableSystem>();
	InteractionTable = std::make_unique<InteractionTableSystem>();

	ObjectGraph = std::make_unique<ObjectGraphSystem>();
	Interactable = std::make_unique<InteractableSystem>();

	Phmo = std::make_unique<PhmoSystem>();
	Vehi = std::make_unique<VehiSystem>();
}

CoreDomainSystem::~CoreDomainSystem() = default;