#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"
#include "Core/States/Infrastructure/Engine/State_Render.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Legacy/System_EventRegistry.h"
#include "Core/Systems/Infrastructure/Persistence/System_Preferences.h"
#include "Core/Systems/Interface/System_Debug.h"
#include <fstream>

void System_Preferences::SavePreferences()
{
	if (!g_pState->Infrastructure->Settings->ShouldUseAppData()) return;

	std::ofstream file(this->GetPreferencesFilePath(), std::ios::trunc);
	if (!file.is_open())
	{
		g_pSystem->Debug->Log("[PreferencesSystem] ERROR: Failed to save user preferences.");
		return;
	}

	file << "; Artemis User Preferences\n";

	this->SaveLifeCycleState(file);
	this->SaveSettingsState(file);
	this->SaveUI(file);

	g_pSystem->Debug->Log("[PreferencesSystem] INFO: User preferences saved successfully.");
}

void System_Preferences::LoadPreferences()
{
	if (!g_pState->Infrastructure->Settings->ShouldUseAppData()) return;

	std::ifstream file(this->GetPreferencesFilePath());
	if (!file.is_open())
	{
		g_pSystem->Debug->Log("[PreferencesSystem] WARNING: No user preferences file found, using defaults.");
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		this->ParseLine(line);
	}

	g_pSystem->Debug->Log("[PreferencesSystem] INFO: User preferences loaded successfully.");
}


void System_Preferences::ParseLine(const std::string& line)
{
	if (line.empty() || line[0] == '#' || line[0] == ';') return;

	auto delimiterPos = line.find('=');
	if (delimiterPos == std::string::npos) return;

	std::string key = line.substr(0, delimiterPos);
	std::string value = line.substr(delimiterPos + 1);

	if (key.find("Lifecycle_") == 0) this->LoadLifecycleState(key, value);
	else if (key.find("Settings_") == 0) this->LoadSettingsState(key, value);
	else if (key.find("UI_") == 0) this->LoadUI(key, value);
}

std::string System_Preferences::GetPreferencesFilePath() const
{
	return g_pState->Infrastructure->Settings->GetAppDataDirectory() + "\\user_preferences.cfg";
}


void System_Preferences::SaveLifeCycleState(std::ofstream& file)
{

}

void System_Preferences::SaveSettingsState(std::ofstream& file)
{
	file << "Settings_ShouldFreezeMouse=" << (g_pState->Infrastructure->Settings->ShouldFreezeMouse() ? "1" : "0") << "\n";
	file << "Settings_ShouldOpenUIOnStart=" << (g_pState->Infrastructure->Settings->ShouldOpenUIOnStart() ? "1" : "0") << "\n";

	file << std::fixed << std::setprecision(2);
	file << "Settings_MenuAlpha=" << g_pState->Infrastructure->Settings->GetMenuAlpha() << "\n";
	file << "Settings_UIScale=" << g_pState->Infrastructure->Render->GetUIScale() << "\n";
	file << std::defaultfloat;
}

void System_Preferences::SaveUI(std::ofstream& file)
{
	file << "UI_LogsAutoScroll=" << (g_pState->Infrastructure->Settings->GetLogsAutoScroll() ? "1" : "0") << "\n";
}

void System_Preferences::LoadLifecycleState(std::string& key, std::string& value)
{

}

void System_Preferences::LoadSettingsState(std::string& key, std::string& value)
{
	if (key == "Settings_ShouldFreezeMouse")
	{
		g_pState->Infrastructure->Settings->SetFreezeMouse(value == "1" || value == "true");
	}
	else if (key == "Settings_ShouldOpenUIOnStart")
	{
		g_pState->Infrastructure->Settings->SetOpenUIOnStart(value == "1" || value == "true");
	}
	else if (key == "Settings_MenuAlpha")
	{
		try
		{
			g_pState->Infrastructure->Settings->SetMenuAlpha(std::stof(value));
		}
		catch (...)
		{
			g_pState->Infrastructure->Settings->SetMenuAlpha(1.0f);
		}
	}
	else if (key == "Settings_UIScale")
	{
		try
		{
			g_pState->Infrastructure->Render->SetUIScale(std::stof(value));
		}
		catch (...)
		{
			g_pState->Infrastructure->Render->SetUIScale(1.0f);
		}
	}
}

void System_Preferences::LoadUI(std::string& key, std::string& value)
{
	if (key == "UI_LogsAutoScroll")
	{
		g_pState->Infrastructure->Settings->SetLogsAutoScroll(value == "1" || value == "true");
	}
}