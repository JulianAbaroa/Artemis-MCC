#include "pch.h"
#include "Core/UI/CoreUI.h"
#include "Core/UI/MainInterface.h"
#include "Core/UI/Tabs/Interface/LogsTab.h"
#include "Core/UI/Tabs/Domain/SettingsTab.h"
#include "Core/UI/Tabs/Domain/Tables/ObjectTableTab.h"
#include "Core/UI/Tabs/Domain/Tables/PlayerTableTab.h"
#include "Core/UI/Tabs/Domain/Graph/ObjectGraphTab.h"
#include "Core/UI/Tabs/Domain/Interactable/InteractableTab.h"
#include "Core/UI/Tabs/Domain/Physics/PhysicsDebugTab.h"

CoreUI::CoreUI()
{
	Main = std::make_unique<MainInterface>();
	Settings = std::make_unique<SettingsTab>();
	Logs = std::make_unique<LogsTab>();

	ObjectTable = std::make_unique<ObjectTableTab>();
	PlayerTable = std::make_unique<PlayerTableTab>();
	ObjectGraph = std::make_unique<ObjectGraphTab>();
	Interactable = std::make_unique<InteractableTab>();
	PhysicsDebug = std::make_unique<PhysicsDebugTab>();
}

CoreUI::~CoreUI() = default;