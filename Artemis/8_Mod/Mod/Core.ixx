module;

#include <windows.h>

export module Core;

import Service.Layer;
import Service.Preferences.System;
import Platform.Layer;
import Map.Layer;
import Resolved.Layer;
import Tables.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Layer;
import DevTools.Type;
import DevTools.Layer;
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
        HMODULE m_Module{};

        // Set once MH_Initialize succeeds. Gates MH_Uninitialize in the destructor.
        bool m_IsMinHookReady{ false };

        Service::Layer m_Service{};
        Platform::Layer m_Platform{ m_Service };

        PreferencesService m_Preferences{ m_Service.m_SettingsStore, m_Service.m_Log };

        DevTools::Type::EngineAccess m_EngineAccess{
            .m_RenderService = m_Platform.m_Render.m_RenderService,
            .m_RenderStore = m_Platform.m_Render.m_RenderStore,
            .m_InputService = m_Platform.m_Render.m_InputService,
            .m_CaptureStore = m_Platform.m_Capture.m_CaptureStore,
            .m_MemoryReaderService = m_Platform.m_Base.m_MemoryReaderService,
            .m_LifecycleService = m_Platform.m_Base.m_LifecycleService,
            .m_LifecycleStore = m_Platform.m_Base.m_LifecycleStore,
            .m_MatchConfigStore = m_Platform.m_Match.m_MatchConfigStore,
            .m_MatchConfigService = m_Platform.m_Match.m_MatchConfigService,
            .m_MatchCycleStore = m_Platform.m_Match.m_MatchCycleStore,
            .m_LocalUserStore = m_Platform.m_Base.m_LocalUserStore,
            .m_VirtualInputStore = m_Platform.m_Control.m_VirtualInputStore,
            .m_PlayerActionStore = m_Platform.m_Control.m_PlayerActionStore,
            .m_BotStore = m_Platform.m_Control.m_BotStore,
            .m_BridgeService = m_Platform.m_Transport.m_BridgeService };

        Map::Layer m_Map{ m_Service.m_Log };
        Resolved::Layer m_Resolved{ m_Service.m_Log, m_Map };
        Tables::Layer m_Tables{ m_Service.m_Log, m_Resolved };
        Environment::Layer m_Environment{ m_Service.m_Log, m_Resolved, m_Tables };
        Egocentric::Layer m_Egocentric{ m_Service.m_Log, m_Resolved, m_Tables, m_Environment };
        Export::Layer m_Export{ m_Resolved, m_Tables, m_Environment, m_Egocentric };
        DevTools::Layer m_DevTools{ m_Service, m_EngineAccess, m_Map, m_Resolved, m_Tables, m_Environment, m_Export, m_Preferences };
        Runtime::Layer m_Runtime{ m_Service, m_Platform, m_Map, m_Resolved, m_Tables, m_Environment, m_Egocentric, m_Export, m_DevTools };
    };
}