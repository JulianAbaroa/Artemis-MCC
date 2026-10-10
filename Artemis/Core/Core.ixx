module;

#include <windows.h>

export module Core;

import Service.Layer;
import Service.Preferences.System;
import Platform.Layer;
import Map.Layer;
import Template.Layer;
import Resolved.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Layer;
import Gui.Layer;
import Viewer.Layer;
import UI.Layer;
import Runtime.Layer;
import std;

export namespace Core
{
    // Root object of the DLL. Owns every layer and the whole lifecycle.
    // note: Layers are built in member order and destroyed in reverse. Each layer takes references to the ones before it.
    class Artemis
    {
    private:
        using PreferencesService = Service::Preferences::System::PreferencesService;

    public:
        // Builds all layers. Initialization happens in Start.
        explicit Artemis(HMODULE handleModule);

        // Saves preferences, signals shutdown, unhooks and releases MinHook.
        ~Artemis();

        // Not copyable or movable. Layers hold references to each other.
        Artemis(const Artemis&) = delete;
        auto operator=(const Artemis&) -> Artemis& = delete;
        Artemis(Artemis&&) = delete;
        auto operator=(Artemis&&) -> Artemis& = delete;

        // Sets up paths, preferences and MinHook, then marks Artemis as running.
        // return: false if MinHook failed to initialize.
        auto Start() -> bool;

        // Runs the runtime layer. Blocks until shutdown.
        // note: Main thread work runs on the calling thread. AI and input threads are spawned.
        auto Run() -> void;

        // Signals every thread to stop. Does not block.
        auto RequestShutdown() -> void;

    private:
        HMODULE m_Module;

        // Set once MH_Initialize succeeds. Gates MH_Uninitialize in the destructor.
        bool m_IsMinHookReady{ false };

        Service::Layer m_Service{};
        Platform::Layer m_Platform{ m_Service };

        PreferencesService m_Preferences{ m_Service.m_SettingsStore, m_Service.m_LogsService };

        Map::Layer m_Map{ m_Service, m_Platform };
        Resolved::Layer m_Resolved{ m_Service, m_Platform, m_Map };
        Template::Layer m_Template{ m_Service, m_Platform };
        Tables::Layer m_Tables{ m_Service, m_Platform, m_Map, m_Template };
        Relations::Layer m_Relations{ m_Service, m_Platform, m_Tables };
        Environment::Layer m_Environment{ m_Service, m_Platform, m_Resolved, m_Tables, m_Relations };
        Egocentric::Layer m_Egocentric{ m_Service, m_Platform, m_Resolved, m_Tables, m_Relations, m_Environment };
        Export::Layer m_Export{ m_Service, m_Platform, m_Resolved, m_Tables, m_Relations, m_Environment, m_Egocentric };
        Gui::Layer m_Gui{ m_Service, m_Platform };
        Viewer::Layer m_Viewer{ m_Service, m_Platform, m_Export, m_Gui, m_Preferences };
        UI::Layer m_UI{ m_Service, m_Platform, m_Resolved, m_Export, m_Gui, m_Viewer };
        Runtime::Layer m_Runtime{ m_Service, m_Platform, m_Map, m_Resolved, m_Template, m_Tables, m_Relations, m_Environment, m_Egocentric, m_Export, m_Viewer.m_CameraStore };
    };
}