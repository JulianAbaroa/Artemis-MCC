#pragma once

class State_Settings;
class System_Logs;

struct Sys_Settings_Deps
{
	State_Settings& State_Settings;
	System_Logs& System_Logs;
};

class System_Settings
{
public:	
	System_Settings(Sys_Settings_Deps deps) : m_Deps(deps) {}
	~System_Settings() = default;

	void InitializePaths(char* buffer);

	void SaveUseAppData();
	void LoadUseAppData();

	void CreateAppData();
	void DeleteAppData();

private:
	Sys_Settings_Deps m_Deps;
};