module;

#include <windows.h>

export module Core;

import Service.Layer;
import Platform.Layer;
import Service.Preferences.System;
import Map.Layer;
import Resolved.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Layer;
import Viewer.Layer;
import UI.Layer;
import Runtime.Layer;
import std;

export namespace Core
{
	class Artemis
	{
	private:
		using PreferencesService = Service::Preferences::System::PreferencesService;

	public:
		explicit Artemis(HMODULE handleModule);
		~Artemis();

		Artemis(const Artemis&) = delete;
		Artemis& operator=(const Artemis&) = delete;
		Artemis(Artemis&&) = delete;
		Artemis& operator=(Artemis&&) = delete;
		
		auto Start() -> bool;
		auto Run() -> void;
		auto RequestShutdown() -> void;

	private:
		HMODULE m_Module;
		bool m_IsMinHookReady{ false };

		Service::Layer m_Service;
		Platform::Layer m_Platform{ m_Service };

		PreferencesService m_Preferences{ m_Platform.m_LifecycleStore, m_Service.m_SettingsStore, m_Service.m_LogsService };

		Map::Layer m_Map{ m_Service, m_Platform };
		Resolved::Layer m_Resolved{ m_Service, m_Platform, m_Map };
		Tables::Layer m_Tables{ m_Service, m_Platform, m_Map };
		Relations::Layer m_Relations{ m_Service, m_Platform, m_Tables };
		Environment::Layer m_Environment{ m_Service, m_Platform, m_Resolved, m_Tables, m_Relations };
		Egocentric::Layer m_Egocentric{ m_Service, m_Platform, m_Resolved, m_Tables, m_Relations, m_Environment };
		Export::Layer m_Export{ m_Service, m_Platform, m_Tables, m_Relations, m_Environment, m_Egocentric };
		Viewer::Layer m_Viewer{ m_Service, m_Platform, m_Resolved, m_Export };
		UI::Layer m_UI{ m_Service, m_Platform, m_Resolved, m_Export, m_Viewer };
		Runtime::Layer m_Runtime{ m_Service, m_Platform, m_Map, m_Resolved, m_Tables, m_Relations, m_Environment, m_Egocentric, m_Export, m_Viewer.m_CameraStore };
	};
}