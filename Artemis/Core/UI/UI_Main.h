#pragma once

class State_Lifecycle;
class State_Render;
class State_Settings;
class System_Debug;
class UI_ObjectTable;
class UI_PlayerTable;
class UI_ObjectGraph;
class UI_Map;
class UI_Interactable;
class UI_Settings;
class UI_MemoryScanner;
class UI_Logs;

struct UI_Main_Dependencies
{
	State_Lifecycle& State_Lifecycle;
	State_Render& State_Render;
	State_Settings& State_Settings;
	System_Debug& System_Debug;
	UI_ObjectTable& UI_ObjectTable;
	UI_PlayerTable& UI_PlayerTable;
	UI_ObjectGraph& UI_ObjectGraph;
	UI_Map& UI_Map;
	UI_Interactable& UI_Interactable;
	UI_Settings& UI_Settings;
	UI_MemoryScanner& UI_MemoryScanner;
	UI_Logs& UI_Logs;
};

class UI_Main
{
public:
	UI_Main(UI_Main_Dependencies dependencies) : m_Deps(dependencies) {}
	~UI_Main() = default;

	void Draw();

private:
	UI_Main_Dependencies m_Deps;

	void HandleWindowReset();
	void DrawStatusBar();
	void DrawTabs();
};