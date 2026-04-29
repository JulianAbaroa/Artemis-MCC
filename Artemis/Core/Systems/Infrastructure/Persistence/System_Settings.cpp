#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Persistence/System_Settings.h"
#include "Core/Systems/Interface/System_Debug.h"
#include <filesystem>
#include <shlobj.h>
#include <fstream>

void System_Settings::InitializePaths(char* buffer)
{
	g_pState->Infrastructure->Settings->SetBaseDirectory(std::string(buffer));
	g_pState->Infrastructure->Settings->SetLoggerPath(g_pState->Infrastructure->Settings->GetBaseDirectory() + "\\Artemis.txt");

	this->LoadUseAppData();

	if (g_pState->Infrastructure->Settings->ShouldUseAppData())
	{
		this->CreateAppData();
	}
}

void System_Settings::SaveUseAppData()
{
	std::string configPath = g_pState->Infrastructure->Settings->GetBaseDirectory() + "\\config.ini";
	std::ofstream file(configPath);
	if (file.is_open())
	{
		file << "useAppData=" << (g_pState->Infrastructure->Settings->ShouldUseAppData() ? "1" : "0") << "\n";
		file.close();
	}
}

void System_Settings::LoadUseAppData()
{
	g_pState->Infrastructure->Settings->SetUseAppData(false);

	std::string configPath = g_pState->Infrastructure->Settings->GetBaseDirectory() + "\\config.ini";
	std::ifstream file(configPath);
	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			if (line.find("useAppData=1") != std::string::npos)
			{
				g_pState->Infrastructure->Settings->SetUseAppData(true);
			}
		}

		file.close();
	}
}

void System_Settings::CreateAppData()
{
	if (!g_pState->Infrastructure->Settings->ShouldUseAppData() ||
		!g_pState->Infrastructure->Settings->IsAppDataDirectoryEmpty()) return;

	PWSTR pathTemp = NULL;

	HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pathTemp);

	if (SUCCEEDED(hr))
	{
		std::filesystem::path basePath(pathTemp);

		basePath /= "Artemis";

		std::error_code errorCode;
		if (std::filesystem::create_directories(basePath, errorCode) ||
			std::filesystem::exists(basePath)) 
		{
			g_pState->Infrastructure->Settings->SetAppDataDirectory(basePath.string());
		}

		CoTaskMemFree(pathTemp);
	}
}

void System_Settings::DeleteAppData()
{
	if (g_pState->Infrastructure->Settings->IsAppDataDirectoryEmpty()) return;

	std::error_code errorCode;

	if (std::filesystem::remove_all(g_pState->Infrastructure->Settings->GetAppDataDirectory(), errorCode) > 0)
	{
		g_pState->Infrastructure->Settings->ClearAppDataDirectory();
	}

	if (errorCode)
	{
		g_pSystem->Debug->Log("[SettingsSystem] ERROR: While deleting AppData. %s.", errorCode.message().c_str());
	}
}