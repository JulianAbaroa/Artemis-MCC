module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Core;

import std;

using namespace std::chrono_literals;

namespace Core
{
    Artemis::Artemis(HMODULE handleModule) : m_Module(handleModule)
    {
        // The lifecycle wiring of the pure components lives here, not in them (D178).
        // Every layer is cleaned up in the order of its dependencies, the ones that read first: Environment (which includes Relations),
        // Egocentric and Export, then Tables, Map and Resolved. The tables read the resolved data, which is built from the map (D195, D200).
        auto& lifecycle = m_Platform.m_Base.m_LifecycleService;

        lifecycle.OnMapFileOpened([this](const std::string& filePath) {
            m_Map.m_MapLoaderService.OpenFile(filePath);
        });

        lifecycle.OnCleanup([this] {
            m_Environment.Cleanup();
            m_Egocentric.Cleanup();
            m_Export.Cleanup();
            m_Tables.Cleanup();
            m_Map.Cleanup();
            m_Resolved.Cleanup();
        });
    }

    auto Artemis::Start() -> bool
    {
        auto& logs = m_Service.m_Log;

        char buffer[MAX_PATH]{};
        GetModuleFileNameA(m_Module, buffer, MAX_PATH);
        std::string directory = std::filesystem::path(buffer).parent_path().string();

        m_Service.m_SettingsService.InitializePaths(directory);
        std::ofstream{ m_Service.m_SettingsStore.GetLoggerPath(), std::ios::trunc };

        if (m_Service.m_SettingsStore.IsAppDataEnabled())
        {
            m_Preferences.Load();
        }

        if (MH_Initialize() != MH_OK)
        {
            logs.Message("[Core] ERROR: MH_Initialize failed.");
            return false;
        }

        m_IsMinHookReady = true;

        m_Platform.m_Base.m_LifecycleStore.SetRunning(true);

        logs.Message("[Core] INFO: Artemis initialized.");
        return true;
    }

    auto Artemis::Run() -> void
    {
        m_Runtime.Run();
    }

    auto Artemis::RequestShutdown() -> void
    {
        m_Platform.m_Base.m_LifecycleService.SignalShutdown();
    }

    Artemis::~Artemis()
    {
        m_Preferences.Save();

        m_Platform.m_Base.m_LifecycleService.SignalShutdown();

        m_Platform.m_Base.m_LifecycleService.RaiseUnhook();

        m_Platform.m_Base.m_LifecycleService.RaiseShutdown();

        // Gives the other threads time to react to the shutdown signal and the raises.
        std::this_thread::sleep_for(200ms);

        if (m_IsMinHookReady) MH_Uninitialize();
    }
}