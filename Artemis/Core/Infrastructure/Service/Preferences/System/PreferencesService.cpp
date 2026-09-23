module Service.Preferences.System;

import std;

namespace Service::Preferences::System
{
	auto PreferencesService::Save() -> void
	{
		if (!m_SettingsStore.ShouldUseAppData()) return;

		std::ofstream file(this->GetPreferencesFilePath(), std::ios::trunc);
		if (!file.is_open())
		{
			m_LogService.Message("[PreferencesService] ERROR: Failed to save user preferences.");
			return;
		}

		file << "; Artemis User Preferences\n";

		this->SaveLifeCycleState(file);
		this->SaveSettingsState(file);
		this->SaveUI(file);

		m_LogService.Message("[PreferencesService] INFO: User preferences saved successfully.");
	}

	auto PreferencesService::Load() -> void
	{
		if (!m_SettingsStore.ShouldUseAppData()) return;

		std::ifstream file(this->GetPreferencesFilePath());
		if (!file.is_open())
		{
			m_LogService.Message("[PreferencesService] WARNING: No user preferences file found, using defaults.");
			return;
		}

		std::string line;
		while (std::getline(file, line))
		{
			this->ParseLine(line);
		}

		m_LogService.Message("[PreferencesService] INFO: User preferences loaded successfully.");
	}

	auto PreferencesService::ParseLine(const std::string& line) -> void
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

	auto PreferencesService::GetPreferencesFilePath() const -> std::string
	{
		return m_SettingsStore.GetAppDataDirectory() + "\\user_preferences.cfg";
	}


	auto PreferencesService::SaveLifeCycleState(std::ofstream& file) -> void
	{

	}

	auto PreferencesService::SaveSettingsState(std::ofstream& file) -> void
	{
		file << "Settings_ShouldFreezeMouse=" << (m_SettingsStore.ShouldFreezeMouse() ? "1" : "0") << "\n";
		file << "Settings_ShouldOpenUIOnStart=" << (m_SettingsStore.ShouldOpenUIOnStart() ? "1" : "0") << "\n";

		file << std::fixed << std::setprecision(2);
		file << "Settings_MenuAlpha=" << m_SettingsStore.GetMenuAlpha() << "\n";
		file << "Settings_UIScale=" << m_SettingsStore.GetUIScale() << "\n";
		file << std::defaultfloat;
	}

	auto PreferencesService::SaveUI(std::ofstream& file) -> void
	{
		file << "UI_LogsAutoScroll=" << (m_SettingsStore.GetLogsAutoScroll() ? "1" : "0") << "\n";
	}

	auto PreferencesService::LoadLifecycleState(std::string& key, std::string& value) -> void
	{

	}

	auto PreferencesService::LoadSettingsState(std::string& key, std::string& value) -> void
	{
		if (key == "Settings_ShouldFreezeMouse")
		{
			m_SettingsStore.SetFreezeMouse(value == "1" || value == "true");
		}
		else if (key == "Settings_ShouldOpenUIOnStart")
		{
			m_SettingsStore.SetOpenUIOnStart(value == "1" || value == "true");
		}
		else if (key == "Settings_MenuAlpha")
		{
			try
			{
				m_SettingsStore.SetMenuAlpha(std::stof(value));
			}
			catch (...)
			{
				m_SettingsStore.SetMenuAlpha(1.0f);
			}
		}
		else if (key == "Settings_UIScale")
		{
			try
			{
				m_SettingsStore.SetUIScale(std::stof(value));
			}
			catch (...)
			{
				m_SettingsStore.SetUIScale(1.0f);
			}
		}
	}

	auto PreferencesService::LoadUI(std::string& key, std::string& value) -> void
	{
		if (key == "UI_LogsAutoScroll")
		{
			m_SettingsStore.SetLogsAutoScroll(value == "1" || value == "true");
		}
	}
}
