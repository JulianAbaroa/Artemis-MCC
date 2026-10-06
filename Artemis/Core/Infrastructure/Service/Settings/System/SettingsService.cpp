module;

#include <cstdlib>

module Service.Settings.System;

import std;

namespace Service::Settings::System
{
    auto SettingsService::InitializePaths(const std::string& directory) -> void
    {
        m_SettingsStore.SetBaseDirectory(directory);
        m_SettingsStore.SetLoggerPath(m_SettingsStore.GetBaseDirectory() + "\\Artemis.txt");

        this->LoadAppDataEnabled();

        if (m_SettingsStore.IsAppDataEnabled())
        {
            this->CreateAppData();
        }
    }

    auto SettingsService::SaveAppDataEnabled() -> void
    {
        std::string configPath = m_SettingsStore.GetBaseDirectory() + "\\config.ini";
        std::ofstream file(configPath);
        if (file.is_open())
        {
            file << "useAppData=" << (m_SettingsStore.IsAppDataEnabled() ? "1" : "0") << "\n";
            file.close();
        }
    }

    auto SettingsService::LoadAppDataEnabled() -> void
    {
        m_SettingsStore.SetAppDataEnabled(false);

        std::string configPath = m_SettingsStore.GetBaseDirectory() + "\\config.ini";
        std::ifstream file(configPath);
        if (file.is_open())
        {
            std::string line;
            while (std::getline(file, line))
            {
                if (line.find("useAppData=1") != std::string::npos)
                {
                    m_SettingsStore.SetAppDataEnabled(true);
                }
            }

            file.close();
        }
    }

    auto SettingsService::CreateAppData() -> void
    {
        if (!m_SettingsStore.IsAppDataEnabled() ||
            !m_SettingsStore.IsAppDataDirectoryEmpty()) return;

        char* envBuffer = nullptr;
        size_t envSize = 0;

        if (_dupenv_s(&envBuffer, &envSize, "LOCALAPPDATA") == 0 && envBuffer != nullptr)
        {
            std::unique_ptr<char, void(*)(void*)> autoFree(envBuffer, std::free);

            std::filesystem::path basePath(envBuffer);
            basePath /= "Artemis";

            std::error_code errorCode;
            if (std::filesystem::create_directories(basePath, errorCode) ||
                std::filesystem::exists(basePath))
            {
                m_SettingsStore.SetAppDataDirectory(basePath.string());
            }
        }
    }

    auto SettingsService::DeleteAppData() -> void
    {
        if (m_SettingsStore.IsAppDataDirectoryEmpty()) return;

        std::error_code errorCode;

        std::filesystem::remove_all(m_SettingsStore.GetAppDataDirectory(), errorCode);

        if (!errorCode)
        {
            m_SettingsStore.ClearAppDataDirectory();
            return;
        }

        m_LogsService.Message("[SettingsService] ERROR: While deleting AppData. {}.", errorCode.message().c_str());
    }
}