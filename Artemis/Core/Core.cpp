module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Core;

import std;

using namespace std::chrono_literals;

namespace Core
{
	Artemis::Artemis(HMODULE handleModule) : m_Module(handleModule) {}

	auto Artemis::Start() -> bool
	{
		auto& logs = m_Service.m_LogsService;

		char buffer[MAX_PATH]{};
		GetModuleFileNameA(m_Module, buffer, MAX_PATH);
		std::string directory = std::filesystem::path(buffer).parent_path().string();

		m_Service.m_SettingsService.InitializePaths(directory.data());
		std::ofstream{ m_Service.m_SettingsStore.GetLoggerPath(), std::ios::trunc };

		if (m_Service.m_SettingsStore.ShouldUseAppData())
		{
			m_Preferences.Load();
		}

		if (MH_Initialize() != MH_OK)
		{
			logs.Message("[Core] ERROR: MH_Initialize failed.");
			return false;
		}

		m_IsMinHookReady = true;

		m_Platform.m_LifecycleStore.SetHandleModule(m_Module);
		m_Platform.m_LifecycleStore.SetRunning(true);

		logs.Message("[Core] INFO: Artemis initialized.");
		return true;
	}

	auto Artemis::Run() -> void
	{
		m_Runtime.Run();
	}

	auto Artemis::RequestShutdown() -> void
	{
		m_Platform.m_LifecycleService.SignalShutdown();
	}

	Artemis::~Artemis()
	{
		m_Platform.m_LifecycleService.SignalShutdown();

		m_Platform.m_LifecycleService.RaiseUnhook();

		m_Platform.m_LifecycleService.RaiseShutdown();

		std::this_thread::sleep_for(200ms);

		if (m_IsMinHookReady) MH_Uninitialize();
	}
}