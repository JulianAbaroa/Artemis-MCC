#pragma once

#include <memory>

class UI_Main;
class UI_Settings;
class UI_Logs;

class UI_ObjectTable;
class UI_PlayerTable;
class UI_ObjectGraph;
class UI_Interactable;

// Main container for the application's UIs.
struct Core_UI
{
	Core_UI();
	~Core_UI();

	std::unique_ptr<UI_Main> Main;
	std::unique_ptr<UI_Settings> Settings;
	std::unique_ptr<UI_Logs> Logs;

	std::unique_ptr<UI_ObjectTable> ObjectTable;
	std::unique_ptr<UI_PlayerTable> PlayerTable;
	std::unique_ptr<UI_ObjectGraph> ObjectGraph;
	std::unique_ptr<UI_Interactable> Interactable;
};

extern Core_UI* g_pUI;