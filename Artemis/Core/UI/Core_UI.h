#pragma once

#include <memory>

// Main.
class UI_Main;

// Object Table.
class UI_ObjectTable;

// Player Table.
class UI_PlayerTable;

// Object Graph.
class UI_ObjectGraph;

// Navigation.
class UI_Navigation;

// Interactable.
class UI_Interactable;

// Map. 
class UI_Map;

// Settings.
class UI_Settings;

// Memory Scanner.
class UI_MemoryScanner;

// Logs.
class UI_Logs;

// Main container for the application's UIs.
struct Core_UI
{
	Core_UI();
	~Core_UI();

	// Main.
	std::unique_ptr<UI_Main> Main;

	// Object Table.
	std::unique_ptr<UI_ObjectTable> ObjectTable;

	// Player Table.
	std::unique_ptr<UI_PlayerTable> PlayerTable;

	// Object Graph.
	std::unique_ptr<UI_ObjectGraph> ObjectGraph;

	// Navigation.
	std::unique_ptr<UI_Navigation> Navigation;

	// Interactable.
	std::unique_ptr<UI_Interactable> Interactable;

	// Map.
	std::unique_ptr<UI_Map> Map;

	// Settings.
	std::unique_ptr<UI_Settings> Settings;

	// Memory Scanner.
	std::unique_ptr<UI_MemoryScanner> MemoryScanner;

	// Logs.
	std::unique_ptr<UI_Logs> Logs;
};

extern Core_UI* g_pUI;