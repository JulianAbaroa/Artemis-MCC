#pragma once

class System_Settings
{
public:	
	void InitializePaths(char* buffer);

	void SaveUseAppData();
	void LoadUseAppData();

	void CreateAppData();
	void DeleteAppData();
};