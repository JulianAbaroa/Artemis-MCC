#include "pch.h"

#include "System_Settings.h"

#include "Core/States/Settings/State_Settings.h"

#include "Core/Systems/Logs/System_Logs.h"

#include <filesystem>
#include <shlobj.h>
#include <fstream>

void System_Settings::InitializePaths(char* buffer)
{
	m_Deps.State_Settings.SetBaseDirectory(std::string(buffer));
	m_Deps.State_Settings.SetLoggerPath(m_Deps.State_Settings.GetBaseDirectory() + "\\Artemis.txt");

	this->LoadUseAppData();

	if (m_Deps.State_Settings.ShouldUseAppData())
	{
		this->CreateAppData();
	}
}

void System_Settings::SaveUseAppData()
{
	std::string configPath = m_Deps.State_Settings.GetBaseDirectory() + "\\config.ini";
	std::ofstream file(configPath);
	if (file.is_open())
	{
		file << "useAppData=" << (m_Deps.State_Settings.ShouldUseAppData() ? "1" : "0") << "\n";
		file.close();
	}
}

void System_Settings::LoadUseAppData()
{
	m_Deps.State_Settings.SetUseAppData(false);

	std::string configPath = m_Deps.State_Settings.GetBaseDirectory() + "\\config.ini";
	std::ifstream file(configPath);
	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			if (line.find("useAppData=1") != std::string::npos)
			{
				m_Deps.State_Settings.SetUseAppData(true);
			}
		}

		file.close();
	}
}

void System_Settings::CreateAppData()
{
	if (!m_Deps.State_Settings.ShouldUseAppData() ||
		!m_Deps.State_Settings.IsAppDataDirectoryEmpty()) return;

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
			m_Deps.State_Settings.SetAppDataDirectory(basePath.string());
		}

		CoTaskMemFree(pathTemp);
	}
}

void System_Settings::DeleteAppData()
{
	if (m_Deps.State_Settings.IsAppDataDirectoryEmpty()) return;

	std::error_code errorCode;

	if (std::filesystem::remove_all(m_Deps.State_Settings.GetAppDataDirectory(), errorCode) > 0)
	{
		m_Deps.State_Settings.ClearAppDataDirectory();
	}

	if (errorCode)
	{
		m_Deps.System_Logs.Log("[SettingsSystem] ERROR: While deleting AppData. %s.", errorCode.message().c_str());
	}
}