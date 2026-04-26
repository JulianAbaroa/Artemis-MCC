#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Legacy/EventRegistrySystem.h"
#include "Core/Systems/Infrastructure/Persistence/PreferencesSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <fstream>

void PreferencesSystem::SavePreferences()
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

void PreferencesSystem::LoadPreferences()
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


void PreferencesSystem::ParseLine(const std::string& line)
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

std::string PreferencesSystem::GetPreferencesFilePath() const
{
	return g_pState->Infrastructure->Settings->GetAppDataDirectory() + "\\user_preferences.cfg";
}


void PreferencesSystem::SaveLifeCycleState(std::ofstream& file)
{

}

void PreferencesSystem::SaveSettingsState(std::ofstream& file)
{
	file << "Settings_ShouldFreezeMouse=" << (g_pState->Infrastructure->Settings->ShouldFreezeMouse() ? "1" : "0") << "\n";
	file << "Settings_ShouldOpenUIOnStart=" << (g_pState->Infrastructure->Settings->ShouldOpenUIOnStart() ? "1" : "0") << "\n";

	file << std::fixed << std::setprecision(2);
	file << "Settings_MenuAlpha=" << g_pState->Infrastructure->Settings->GetMenuAlpha() << "\n";
	file << "Settings_UIScale=" << g_pState->Infrastructure->Render->GetUIScale() << "\n";
	file << std::defaultfloat;
}

void PreferencesSystem::SaveUI(std::ofstream& file)
{
	file << "UI_LogsAutoScroll=" << (g_pState->Infrastructure->Settings->GetLogsAutoScroll() ? "1" : "0") << "\n";
}

void PreferencesSystem::LoadLifecycleState(std::string& key, std::string& value)
{

}

void PreferencesSystem::LoadSettingsState(std::string& key, std::string& value)
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

void PreferencesSystem::LoadUI(std::string& key, std::string& value)
{
	if (key == "UI_LogsAutoScroll")
	{
		g_pState->Infrastructure->Settings->SetLogsAutoScroll(value == "1" || value == "true");
	}
}