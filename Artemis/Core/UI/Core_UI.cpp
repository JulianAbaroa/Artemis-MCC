#include "pch.h"

// Header.
#include "Core_UI.h"

#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Interface/Core_State_Interface.h"

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Interface/Core_System_Interface.h"

// --- UI ---

// Main.
#include "Core/UI/UI_Main.h"

// Object Table.
#include "Core/UI/Domain/Object/UI_ObjectTable.h"

// Player Table.
#include "Core/UI/Domain/Player/UI_PlayerTable.h"

// Object Graph.
#include "Core/UI/Domain/Graph/UI_ObjectGraph.h"

// Navigation.
#include "Core/UI/Domain/Navigation/UI_Navigation.h"

// Environment.
#include "Core/UI/Domain/Environment/UI_Environment.h"

// Interactable.
#include "Core/UI/Domain/Interactable/UI_Interactable.h"

// Map.
#include "Core/UI/Domain/Map/UI_Map.h"

// Settings.
#include "Core/UI/Infrastructure/Persistence/UI_Settings.h"

// Memory Scanner.
#include "Core/UI/Infrastructure/Memory/UI_MemoryScanner.h"

// Logs.
#include "Core/UI/Interface/UI_Logs.h"

Core_UI::Core_UI() = default;
Core_UI::~Core_UI() = default;

void Core_UI::Initialize(Core_State& state, Core_System& system)
{
	// Object Table.
	ObjectTable = std::make_unique<UI_ObjectTable>(
		*state.Domain->ObjectTable);
	
	// Player Table.
	PlayerTable = std::make_unique<UI_PlayerTable>(
		UI_PlayerTable_Dependencies {
			.State_PlayerTable = *state.Domain->PlayerTable,
			.System_Debug = *system.Interface->Debug,
		}
	);

	// Object Graph.
	ObjectGraph = std::make_unique<UI_ObjectGraph>(
		UI_ObjectGraph_Dependencies {
			.State_ObjectTable = *state.Domain->ObjectTable,
			.State_PlayerTable = *state.Domain->PlayerTable,
			.State_ObjectGraph = *state.Domain->ObjectGraph,
			.State_PlayerGraph = *state.Domain->PlayerGraph,
			.System_Debug = *system.Interface->Debug,
		}
	);

	// Navigation.
	Navigation = std::make_unique<UI_Navigation>(
		*state.Domain->Navigation);

	// Environment.
	Environment = std::make_unique<UI_Environment>(
		UI_Environment_Dependencies {
			.State_Environment = *state.Domain->Environment,
			.System_Debug = *system.Interface->Debug,
		}
	);

	// Interactable
	Interactable = std::make_unique<UI_Interactable>(
		UI_Interactable_Dependencies {
			.State_ObjectTable = *state.Domain->ObjectTable,
			.State_InteractionTable = *state.Domain->InteractionTable,
			.State_Interactable = *state.Domain->Interactable,
		}
	);

	// Map.
	Map = std::make_unique<UI_Map>(
		UI_Map_Dependencies {
			.UI_Navigation = *Navigation,
			.UI_Environment = *Environment,
		}
	);

	// Settings.
	Settings = std::make_unique<UI_Settings>(
		UI_Settings_Dependencies {
			.State_Render = *state.Infrastructure->Render,
			.State_Settings = *state.Infrastructure->Settings,
			.System_Settings = *system.Infrastructure->Settings,
			.System_Preferences = *system.Infrastructure->Preferences,
			.System_Debug = *system.Interface->Debug,
		}
	);

	// Memory Scanner.
	MemoryScanner = std::make_unique<UI_MemoryScanner>(
		UI_MemoryScanner_Dependencies {
			.State_MemoryScanner = *state.Infrastructure->MemoryScanner,
			.System_MemoryScanner = *system.Infrastructure->MemoryScanner,
		}
	);

	// Logs.
	Logs = std::make_unique<UI_Logs>(
		UI_Logs_Dependencies {
			.State_Settings = *state.Infrastructure->Settings,
			.State_Debug = *state.Interface->Debug,
			.System_Debug = *system.Interface->Debug,
		}
	);

	// Main.
	Main = std::make_unique<UI_Main>(
		UI_Main_Dependencies {
			.State_Lifecycle = *state.Infrastructure->Lifecycle,
			.State_Render = *state.Infrastructure->Render,
			.State_Settings = *state.Infrastructure->Settings,
			.System_Debug = *system.Interface->Debug,
			.UI_ObjectTable = *ObjectTable,
			.UI_PlayerTable = *PlayerTable,
			.UI_ObjectGraph = *ObjectGraph,
			.UI_Map = *Map,
			.UI_Interactable = *Interactable,
			.UI_Settings = *Settings,
			.UI_MemoryScanner = *MemoryScanner,
			.UI_Logs = *Logs,
		}
	);
}

void Core_UI::Shutdown()
{
	ObjectTable.reset();
	PlayerTable.reset();
	ObjectGraph.reset();
	Navigation.reset();
	Environment.reset();
	Interactable.reset();
	Map.reset();
	Settings.reset();
	MemoryScanner.reset();
	Logs.reset();
	Main.reset();
}