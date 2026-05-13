#include "pch.h"

// Header.
#include "Core_UI.h"

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

Core_UI::Core_UI()
{
	// Main.
	Main = std::make_unique<UI_Main>();

	// Object Table.
	ObjectTable = std::make_unique<UI_ObjectTable>();
	
	// Player Table.
	PlayerTable = std::make_unique<UI_PlayerTable>();

	// Object Graph.
	ObjectGraph = std::make_unique<UI_ObjectGraph>();

	// Navigation.
	Navigation = std::make_unique<UI_Navigation>();

	// Interactable
	Interactable = std::make_unique<UI_Interactable>();

	// Map.
	Map = std::make_unique<UI_Map>();

	// Settings.
	Settings = std::make_unique<UI_Settings>();

	// Memory Scanner.
	MemoryScanner = std::make_unique<UI_MemoryScanner>();

	// Logs.
	Logs = std::make_unique<UI_Logs>();
}

Core_UI::~Core_UI() = default;