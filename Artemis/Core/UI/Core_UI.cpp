#include "pch.h"

#include "Core_UI.h"

#include "Core/States/Core_State.h"

#include "Core/Systems/Core_System.h"

#include "Launcher/UI_Tab.h"

#include "Launcher/UI_Launcher.h"
#include "Overlay/UI_Overlay.h"
#include "ObjectTable/UI_ObjectTable.h"
#include "PlayerTable/UI_PlayerTable.h"
#include "Map/Navigation/UI_Navigation.h"
#include "Map/Environment/UI_Environment.h"
#include "Map/UI_Map.h"
#include "Settings/UI_Settings.h"
#include "MemoryScanner/UI_MemoryScanner.h"
#include "Logs/UI_Logs.h"

#include <vector>

Core_UI::Core_UI() = default;
Core_UI::~Core_UI() = default;

void Core_UI::Initialize(Core_State& state, Core_System& system)
{
	ObjectTable = std::make_unique<UI_ObjectTable>(
		*state.ObjectTable);

	PlayerTable = std::make_unique<UI_PlayerTable>(
		*state.PlayerTable);

	Navigation = std::make_unique<UI_Navigation>(
		UI_Navigation_Dependencies {
			.State_Navigation = *state.Navigation,
			.System_Logs = *system.Logs,
		});

	Environment = std::make_unique<UI_Environment>(
		UI_Environment_Dependencies {
			.State_Environment = *state.Environment,
			.System_Logs = *system.Logs,
		});

	//Interactable = std::make_unique<UI_Interactable>(
	//	UI_Interactable_Dependencies{
	//		.State_ObjectTable = *state.Domain->ObjectTable,
	//		.State_InteractionTable = *state.Domain->InteractionTable,
	//		.State_Interactable = *state.Domain->Interactable,
	//	});

	Map = std::make_unique<UI_Map>(
		UI_Map_Dependencies {
			.UI_Navigation = *Navigation,
			.UI_Environment = *Environment,
			.System_Logs = *system.Logs,
		});

	Settings = std::make_unique<UI_Settings>(
		UI_Settings_Dependencies {
			.State_Render = *state.Render,
			.State_Settings = *state.Settings,
			.System_Settings = *system.Settings,
			.System_Logs = *system.Logs,
		});

	MemoryScanner = std::make_unique<UI_MemoryScanner>(
		UI_MemoryScanner_Dependencies {
			.State_MemoryScanner = *state.Memory,
			.System_MemoryScanner = *system.MemoryScanner,
		});

	Logs = std::make_unique<UI_Logs>(
		UI_Logs_Dependencies{
			.State_Settings = *state.Settings,
			.State_Logs = *state.Logs,
			.System_Logs = *system.Logs,
		});


	Launcher = std::make_unique<UI_Launcher>(
		UI_Launcher_Dependencies { 
			.Tabs = {
				ObjectTable.get(),
				PlayerTable.get(),
				Map.get(),
				Settings.get(),
				MemoryScanner.get(),
				Logs.get()
			},
			.State_Render = *state.Render,
		});

	Overlay = std::make_unique<UI_Overlay>(
		UI_Overlay_Dependencies {
			.State_Lifecycle = *state.Lifecycle,
			.State_Render = *state.Render,
		});
}

void Core_UI::Deinitialize()
{
	Launcher.reset();
	Overlay.reset();
	ObjectTable.reset();
	PlayerTable.reset();
	Navigation.reset();
	Environment.reset();
	//Interactable.reset();
	Map.reset();
	Settings.reset();
	MemoryScanner.reset();
	Logs.reset();
}