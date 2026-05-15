#pragma once

class State_Settings;
class System_Debug;

struct System_Settings_Dependencies
{
	State_Settings& State_Settings;
	System_Debug& System_Debug;
};

class System_Settings
{
public:	
	System_Settings(System_Settings_Dependencies dependencies) : 
		m_Deps(dependencies) {}
	~System_Settings() = default;

	void InitializePaths(char* buffer);

	void SaveUseAppData();
	void LoadUseAppData();

	void CreateAppData();
	void DeleteAppData();

private:
	System_Settings_Dependencies m_Deps;
};