#pragma once

#include <memory>

class Core_State;
class Core_System;

class UI_Launcher;
class UI_Overlay;
class UI_ObjectTable;
class UI_PlayerTable;
class UI_Navigation;
class UI_Environment;
class UI_Map;
class UI_Settings;
class UI_MemoryScanner;
class UI_Logs;

class Core_UI
{
public:
	Core_UI();
	~Core_UI();

	void Initialize(Core_State& state, Core_System& system);
	void Deinitialize();

	std::unique_ptr<UI_Launcher> Launcher;
	std::unique_ptr<UI_Overlay> Overlay;
	std::unique_ptr<UI_ObjectTable> ObjectTable;
	std::unique_ptr<UI_PlayerTable> PlayerTable;
	std::unique_ptr<UI_Navigation> Navigation;
	std::unique_ptr<UI_Environment> Environment;
	std::unique_ptr<UI_Map> Map;
	std::unique_ptr<UI_Settings> Settings;
	std::unique_ptr<UI_MemoryScanner> MemoryScanner;
	std::unique_ptr<UI_Logs> Logs;
};