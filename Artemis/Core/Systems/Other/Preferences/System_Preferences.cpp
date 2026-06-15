#include "pch.h"

#include "System_Preferences.h"

#include "Core/States/Other/Lifecycle/State_Lifecycle.h"
#include "Core/States/Other/Settings/State_Settings.h"
#include "Core/States/Other/Render/State_Render.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include <fstream>

void System_Preferences::SavePreferences()
{
	if (!m_Deps.State_Settings.ShouldUseAppData()) return;

	std::ofstream file(this->GetPreferencesFilePath(), std::ios::trunc);
	if (!file.is_open())
	{
		m_Deps.System_Logs.Log("[PreferencesSystem] ERROR: Failed to save user preferences.");
		return;
	}

	file << "; Artemis User Preferences\n";

	this->SaveLifeCycleState(file);
	this->SaveSettingsState(file);
	this->SaveUI(file);

	m_Deps.System_Logs.Log("[PreferencesSystem] INFO: User preferences saved successfully.");
}

void System_Preferences::LoadPreferences()
{
	if (!m_Deps.State_Settings.ShouldUseAppData()) return;

	std::ifstream file(this->GetPreferencesFilePath());
	if (!file.is_open())
	{
		m_Deps.System_Logs.Log("[PreferencesSystem] WARNING: No user preferences file found, using defaults.");
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		this->ParseLine(line);
	}

	m_Deps.System_Logs.Log("[PreferencesSystem] INFO: User preferences loaded successfully.");
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
	return m_Deps.State_Settings.GetAppDataDirectory() + "\\user_preferences.cfg";
}


void System_Preferences::SaveLifeCycleState(std::ofstream& file)
{

}

void System_Preferences::SaveSettingsState(std::ofstream& file)
{
	file << "Settings_ShouldFreezeMouse=" << (m_Deps.State_Settings.ShouldFreezeMouse() ? "1" : "0") << "\n";
	file << "Settings_ShouldOpenUIOnStart=" << (m_Deps.State_Settings.ShouldOpenUIOnStart() ? "1" : "0") << "\n";

	file << std::fixed << std::setprecision(2);
	file << "Settings_MenuAlpha=" << m_Deps.State_Settings.GetMenuAlpha() << "\n";
	file << "Settings_UIScale=" << m_Deps.State_Render.GetUIScale() << "\n";
	file << std::defaultfloat;
}

void System_Preferences::SaveUI(std::ofstream& file)
{
	file << "UI_LogsAutoScroll=" << (m_Deps.State_Settings.GetLogsAutoScroll() ? "1" : "0") << "\n";
}

void System_Preferences::LoadLifecycleState(std::string& key, std::string& value)
{

}

void System_Preferences::LoadSettingsState(std::string& key, std::string& value)
{
	if (key == "Settings_ShouldFreezeMouse")
	{
		m_Deps.State_Settings.SetFreezeMouse(value == "1" || value == "true");
	}
	else if (key == "Settings_ShouldOpenUIOnStart")
	{
		m_Deps.State_Settings.SetOpenUIOnStart(value == "1" || value == "true");
	}
	else if (key == "Settings_MenuAlpha")
	{
		try
		{
			m_Deps.State_Settings.SetMenuAlpha(std::stof(value));
		}
		catch (...)
		{
			m_Deps.State_Settings.SetMenuAlpha(1.0f);
		}
	}
	else if (key == "Settings_UIScale")
	{
		try
		{
			m_Deps.State_Render.SetUIScale(std::stof(value));
		}
		catch (...)
		{
			m_Deps.State_Render.SetUIScale(1.0f);
		}
	}
}

void System_Preferences::LoadUI(std::string& key, std::string& value)
{
	if (key == "UI_LogsAutoScroll")
	{
		m_Deps.State_Settings.SetLogsAutoScroll(value == "1" || value == "true");
	}
}