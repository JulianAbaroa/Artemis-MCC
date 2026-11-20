module;

#include <windows.h>

export module Mod.Layer;

import Services.Layer;
import Services.Preferences.System;
import Engine.Base.Layer;
import Engine.Capture.Type;
import Engine.Capture.Layer;
import Engine.Control.Layer;
import Engine.Match.Layer;
import Engine.Render.Layer;
import Bridge.Transport.Layer;
import Blam.Map.Layer;
import Blam.MapModel.Layer;
import Blam.Runtime.Layer;
import Perception.Layer;
import Views.Layer;
import Bridge.Codec.Layer;
import Bridge.Codec.Pipeline.Type;
import DevTools.Viewer.Access.Type;
import DevTools.Gui.Layer;
import DevTools.Viewer.Layer;
import DevTools.Workbench.Layer;
import Mod.Port.System;
import Mod.Thread.System;
import std;

namespace
{
    using Engine::Capture::Type::k_TickQueueDepth;
}

export namespace Mod
{
    // Root of the DLL. Owns every Layer, the lifecycle wiring of the engine and the threads.
    // note: Built in member order and destroyed in reverse: each Layer takes references to the ones before it, and the threads are the
    // last members, so they are joined first.
    class Layer
    {
    private:
        using milliseconds = std::chrono::milliseconds;

        using PreferencesService = Services::Preferences::System::PreferencesService;

        using BridgeEngineAdapter = Mod::Port::System::BridgeEngineAdapter;
        using BridgeActionSource = Mod::Port::System::BridgeActionSource;
        using TickProducerAdapter = Mod::Port::System::TickProducerAdapter;

        using ObservedTick = Bridge::Codec::Pipeline::Type::ObservedTick;
        using TickQueue = Bridge::Codec::Pipeline::Type::TickQueue;

        using MainThread = Mod::Thread::System::MainThread;
        using AIThread = Mod::Thread::System::AIThread;
        using BenchThread = Mod::Thread::System::BenchThread;
        using BridgeThread = Mod::Thread::System::BridgeThread;
        using DumpThread = Mod::Thread::System::DumpThread;

    public:
        // Builds all layers. Initialization happens in Start.
        explicit Layer(HMODULE handleModule);

        // Saves preferences, signals shutdown, unhooks and releases MinHook.
        ~Layer();

        // Not copyable or movable. Layers hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;
        Layer(Layer&&) = delete;
        auto operator=(Layer&&) -> Layer& = delete;

        // Sets up paths, preferences and MinHook, then marks Artemis as running.
        // return: false if MinHook failed to initialize.
        auto Start() -> bool;

        // Starts the AI, bench, bridge and dump threads and runs the main thread until the lifecycle stops.
        // note: Blocks. The main thread work runs on the calling thread.
        auto Run() -> void;

        // Signals every thread to stop. Does not block.
        auto RequestShutdown() -> void;

    private:
        HMODULE m_Module{};

        // Set once MH_Initialize succeeds. Gates MH_Uninitialize in the destructor.
        bool m_IsMinHookReady{ false };

        Services::Layer m_Service{};

        // The Layers of the engine in the order they are built, then the port adapters and the transport.
        Engine::Base::Layer m_Base{ m_Service };
        Engine::Capture::Layer m_Capture{ m_Service, m_Base };
        Engine::Control::Layer m_Control{ m_Service, m_Base };
        Engine::Match::Layer m_Match{ m_Service, m_Base };
        Engine::Render::Layer m_Render{ m_Service, m_Base };
        BridgeEngineAdapter m_BridgeEngineAdapter{ m_Base.m_LifecycleStore, m_Match.m_MatchCycleStore, m_Control.m_PlayerActionStore,
            m_Base.m_MatchSetupService, m_Base.m_PresetService, m_Base.m_MapVariantService, m_Match.m_MatchConfigService };
        Bridge::Transport::Layer m_Transport{ m_Service, m_BridgeEngineAdapter };
        BridgeActionSource m_BridgeActionSource{ m_Transport.m_BridgeService };

        // The ticks the game thread copies for the AI thread (D305), and the producer that fills the queue from the tick hooks.
        TickQueue m_TickQueue{ k_TickQueueDepth };
        TickProducerAdapter m_TickProducer{ m_Base.m_LifecycleStore, m_Capture.m_CaptureService, m_Transport.m_BridgeService, m_TickQueue };

        // Registers the lifecycle callbacks of the engine. A member and not the body of the constructor so they are registered before the
        // tools build theirs: LifecycleService calls them in registration order (M8 C, sec. 2.5).
        bool m_IsEngineWired{ this->WireEngine() };

        PreferencesService m_Preferences{ m_Service.m_SettingsStore, m_Service.m_Log };

        DevTools::Viewer::Access::Type::EngineAccess m_EngineAccess{
            .m_RenderService = m_Render.m_RenderService,
            .m_RenderStore = m_Render.m_RenderStore,
            .m_InputService = m_Render.m_InputService,
            .m_CaptureStore = m_Capture.m_CaptureStore,
            .m_OracleStore = m_Capture.m_OracleStore,
            .m_MemoryReaderService = m_Base.m_MemoryReaderService,
            .m_GameCameraService = m_Base.m_GameCameraService,
            .m_FrameClockReader = m_Base.m_FrameClockReader,
            .m_LifecycleService = m_Base.m_LifecycleService,
            .m_LifecycleStore = m_Base.m_LifecycleStore,
            .m_MatchConfigStore = m_Match.m_MatchConfigStore,
            .m_MatchConfigService = m_Match.m_MatchConfigService,
            .m_MapVariantDecodeStore = m_Match.m_MapVariantDecodeStore,
            .m_MapVariantCatalogService = m_Match.m_MapVariantCatalogService,
            .m_MatchCycleStore = m_Match.m_MatchCycleStore,
            .m_MatchSetupService = m_Base.m_MatchSetupService,
            .m_PresetService = m_Base.m_PresetService,
            .m_MapVariantService = m_Base.m_MapVariantService,
            .m_LocalUserStore = m_Base.m_LocalUserStore,
            .m_VirtualInputStore = m_Control.m_VirtualInputStore,
            .m_PlayerActionStore = m_Control.m_PlayerActionStore,
            .m_BotStore = m_Control.m_BotStore,
            .m_BridgeService = m_Transport.m_BridgeService,
            .m_PovRequestStore = m_Base.m_PovRequestStore,
            .m_PovStatusStore = m_Base.m_PovStatusStore };

        Blam::Map::Layer m_Map{ m_Service.m_Log };
        Blam::MapModel::Layer m_Resolved{ m_Service.m_Log, m_Map };
        Blam::Runtime::Layer m_Tables{ m_Service.m_Log, m_Resolved };
        Perception::Layer m_Environment{ m_Service.m_Log, m_Resolved, m_Tables };
        Views::Layer m_Egocentric{ m_Service.m_Log, m_Resolved, m_Tables, m_Environment };
        Bridge::Codec::Layer m_Export{ m_Resolved, m_Tables, m_Environment, m_Egocentric };

        // The development tools, in the order they are built.
        DevTools::Gui::Layer m_Gui{ m_Service, m_EngineAccess.m_RenderService, m_EngineAccess.m_RenderStore };
        DevTools::Viewer::Layer m_Viewer{ m_Service, m_EngineAccess, m_Environment, m_Gui, m_Preferences };
        DevTools::Workbench::Layer m_UI{ m_Service, m_EngineAccess, m_Map, m_Resolved, m_Tables, m_Export, m_Gui, m_Viewer, m_Preferences };

        MainThread m_Main{ m_Service, m_Base, m_Match, m_Render };
        AIThread m_AI{ m_Service, m_Base, m_Capture, m_Transport, m_Map, m_Resolved, m_Tables, m_Environment, m_Egocentric, m_Export, m_TickQueue };
        BenchThread m_Bench{ m_Service, m_Base, m_UI.m_BenchUIService };
        BridgeThread m_Bridge{ m_Service, m_Base, m_Transport, m_Export };
        DumpThread m_Dump{ m_Service, m_Base, m_UI.m_TickDumpUIService };

        // Last members: they are joined first when the Layer is destroyed.
        std::jthread m_AIThread{};
        std::jthread m_BenchThread{};
        std::jthread m_BridgeThread{};
        std::jthread m_DumpThread{};

        static constexpr milliseconds k_RenderShutdownTimeout{ 500 };

        // Longest wait for the game thread to give the view back to the local player at the unload (VW.9a).
        static constexpr milliseconds k_PovReleaseTimeout{ 250 };

        // Connects the action source and the lifecycle callbacks of the engine, before the tools and the pure layers register
        // theirs (OnEngineInitialized, OnUnhook, OnCleanup, Render OnInitialized, OnShutdown, in that order).
        // return: Always true, it only gives m_IsEngineWired a value.
        auto WireEngine() -> bool;

        // Gives each tool the tick that just went to the bridge.
        // note: Runs on the AI thread after the push to the bridge (D223). The camera state is read once, so the scene and the recording agree.
        auto OnTick(const ObservedTick& tick) -> void;

        // Removes the input hooks, then asks the render to shut down on its next Present and waits for it.
        // The render hooks are removed after that, and the shutdown is forced if it did not finish in time.
        auto ShutdownRenderAndInput() -> void;

        // Installs the hooks on the game manager of MCC (local users, input) and on the session checks of the engine.
        // note: A failed hook is logged and the rest still go in.
        auto InstallHostHooks() -> void;

        // Removes the host hooks in reverse order.
        auto UninstallHostHooks() -> void;
    };
}