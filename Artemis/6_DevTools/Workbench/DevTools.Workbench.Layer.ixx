export module DevTools.Workbench.Layer;

import Services.Layer;
import Services.Preferences.System;
import Engine.Render.Window.Type;
import Engine.Render.Type;
import Blam.Map.Layer;
import Blam.MapModel.Layer;
import Blam.Runtime.Layer;
import Bridge.Codec.Layer;
import DevTools.Viewer.Access.Type;
import DevTools.Gui.Layer;
import DevTools.Gui.Backend.System;
import DevTools.Viewer.Layer;

import DevTools.Workbench.Hotkey.State;
import DevTools.Workbench.Hotkey.System;
import DevTools.Workbench.Launcher.State;
import DevTools.Workbench.Launcher.System;
import DevTools.Workbench.ObjectTable.State;
import DevTools.Workbench.ObjectTable.System;
import DevTools.Workbench.PlayerTable.State;
import DevTools.Workbench.PlayerTable.System;
import DevTools.Workbench.DefinitionsInspector.System;
import DevTools.Workbench.Settings.System;
import DevTools.Workbench.Capture.State;
import DevTools.Workbench.Capture.System;
import DevTools.Workbench.Simulation.State;
import DevTools.Workbench.Simulation.System;
import DevTools.Workbench.TickTrace.State;
import DevTools.Workbench.TickTrace.System;
import DevTools.Workbench.TickDump.State;
import DevTools.Workbench.TickDump.System;
import DevTools.Workbench.Bench.State;
import DevTools.Workbench.Bench.System;
import DevTools.Workbench.Logs.State;
import DevTools.Workbench.Logs.System;
import std;

export namespace DevTools::Workbench
{
    // Root of the UI layer. Owns the stores and the services of the launcher and its tab windows.
    // note: Hooks into the Platform events. It draws the windows with the game frame and handles the hotkeys of the window messages.
    class Layer
    {
    private:
        using PreferencesService = Services::Preferences::System::PreferencesService;

        using FrameContext = Engine::Render::Type::FrameContext;
        using WindowMessage = Engine::Render::Window::Type::WindowMessage;

        using BackendGuiService = DevTools::Gui::Backend::System::BackendGuiService;

        using HotkeyUIStore = DevTools::Workbench::Hotkey::State::HotkeyUIStore;
        using LauncherUIStore = DevTools::Workbench::Launcher::State::LauncherUIStore;
        using ObjectTableUIStore = DevTools::Workbench::ObjectTable::State::ObjectTableUIStore;
        using PlayerTableUIStore = DevTools::Workbench::PlayerTable::State::PlayerTableUIStore;
        using CaptureUIStore = DevTools::Workbench::Capture::State::CaptureUIStore;
        using SimulationUIStore = DevTools::Workbench::Simulation::State::SimulationUIStore;
        using TickTraceUIStore = DevTools::Workbench::TickTrace::State::TickTraceUIStore;
        using TickDumpUIStore = DevTools::Workbench::TickDump::State::TickDumpUIStore;
        using BenchUIStore = DevTools::Workbench::Bench::State::BenchUIStore;
        using LogsUIStore = DevTools::Workbench::Logs::State::LogsUIStore;

        using HotkeyUIService = DevTools::Workbench::Hotkey::System::HotkeyUIService;
        using LauncherUIService = DevTools::Workbench::Launcher::System::LauncherUIService;
        using ObjectTableUIService = DevTools::Workbench::ObjectTable::System::ObjectTableUIService;
        using PlayerTableUIService = DevTools::Workbench::PlayerTable::System::PlayerTableUIService;
        using DefinitionsInspectorUIService = DevTools::Workbench::DefinitionsInspector::System::DefinitionsInspectorUIService;
        using SettingsUIService = DevTools::Workbench::Settings::System::SettingsUIService;
        using CaptureUIService = DevTools::Workbench::Capture::System::CaptureUIService;
        using SimulationUIService = DevTools::Workbench::Simulation::System::SimulationUIService;
        using TickTraceUIService = DevTools::Workbench::TickTrace::System::TickTraceUIService;
        using TickDumpUIService = DevTools::Workbench::TickDump::System::TickDumpUIService;
        using BenchUIService = DevTools::Workbench::Bench::System::BenchUIService;
        using LogsUIService = DevTools::Workbench::Logs::System::LogsUIService;

    public:
        Layer(Services::Layer& service, DevTools::Viewer::Access::Type::EngineAccess& engine, Blam::Map::Layer& map, Blam::MapModel::Layer& resolved,
            Blam::Runtime::Layer& tables, Bridge::Codec::Layer& exportLayer, DevTools::Gui::Layer& gui, DevTools::Viewer::Layer& viewer,
            PreferencesService& preferences) :
            m_BackendGuiService(gui.m_BackendGuiService),
            m_HotkeyUIService(service.m_SettingsStore, m_HotkeyUIStore, m_LauncherUIStore, m_CaptureUIStore, viewer.m_OverlayStore),
            m_LauncherUIService(service.m_SettingsStore, m_LauncherUIStore),
            m_ObjectTableUIService(engine.m_CaptureStore, exportLayer.m_TickStore, m_ObjectTableUIStore, m_CaptureUIStore),
            m_PlayerTableUIService(engine.m_CaptureStore, exportLayer.m_TickStore, m_PlayerTableUIStore),
            m_DefinitionsInspectorUIService(exportLayer.m_TickStore, resolved.m_DefinitionsStore, viewer.m_SelectionStore, service.m_SettingsStore),
            m_SettingsUIService(service.m_SettingsStore, service.m_SettingsService, service.m_Log, m_HotkeyUIStore, viewer.m_OptionsStore, preferences, viewer.m_CameraStore),
            m_CaptureUIService(service.m_Log, service.m_SettingsStore, engine.m_MemoryReaderService, resolved.m_DefinitionsStore, engine.m_CaptureStore, exportLayer.m_TickStore, m_CaptureUIStore),
            m_SimulationUIService(service.m_Log, service.m_TelemetryStore, engine.m_LifecycleStore, engine.m_MatchConfigStore,
                engine.m_MatchConfigService, engine.m_LocalUserStore, engine.m_VirtualInputStore, engine.m_PlayerActionStore,
                engine.m_BotStore, engine.m_BridgeService, exportLayer.m_TickStore, m_SimulationUIStore),
            m_TickTraceUIService(service.m_Log, service.m_SettingsStore, map.m_FileStore, tables.m_ObjectStore,
                m_TickTraceUIStore),
            m_TickDumpUIService(service.m_Log, service.m_SettingsStore, map.m_FileStore, exportLayer.m_FlatService,
                m_TickDumpUIStore),
            m_LogsUIService(service.m_SettingsStore, service.m_LogsStore, m_LogsUIStore),
            m_BenchUIService(service.m_LogsStore, service.m_Log, service.m_SettingsStore, engine.m_MemoryReaderService,
                engine.m_LocalUserStore, engine.m_PlayerActionStore, engine.m_VirtualInputStore, engine.m_LifecycleStore,
                engine.m_MatchConfigStore, engine.m_MatchCycleStore, engine.m_MatchConfigService, engine.m_BotStore,
                exportLayer.m_TickStore, m_TickTraceUIStore, m_BenchUIStore)
        {
            engine.m_RenderService.OnInitialized([this](const FrameContext& frame) {
                this->Initialize(frame);
            });

            engine.m_RenderService.OnFrame([this](const FrameContext& frame) {
                this->DrawFrame(frame);
            });

            engine.m_RenderService.OnShutdown([this] {
                this->Shutdown();
            });

            // The hotkeys run before every other handler, so the viewer cannot swallow them.
            engine.m_InputService.OnWindowMessage([this](WindowMessage& message) {
                return m_HotkeyUIService.HandleMessage(message);
            }, true);

            engine.m_InputService.OnWindowMessage([&gui](WindowMessage& message) {
                return gui.m_WndProcGuiService.HandleMessage(message);
            });
        }
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // Backend of the Gui layer, used to know if the UI can be drawn.
        BackendGuiService& m_BackendGuiService;

        // --- State ---
        HotkeyUIStore m_HotkeyUIStore{};
        LauncherUIStore m_LauncherUIStore{};
        ObjectTableUIStore m_ObjectTableUIStore{};
        PlayerTableUIStore m_PlayerTableUIStore{};
        CaptureUIStore m_CaptureUIStore{};
        SimulationUIStore m_SimulationUIStore{};
        TickTraceUIStore m_TickTraceUIStore{};
        TickDumpUIStore m_TickDumpUIStore{};
        LogsUIStore m_LogsUIStore{};
        BenchUIStore m_BenchUIStore{};

        // --- System ---
        HotkeyUIService m_HotkeyUIService;
        LauncherUIService m_LauncherUIService;
        ObjectTableUIService m_ObjectTableUIService;
        PlayerTableUIService m_PlayerTableUIService;
        DefinitionsInspectorUIService m_DefinitionsInspectorUIService;
        SettingsUIService m_SettingsUIService;
        CaptureUIService m_CaptureUIService;
        SimulationUIService m_SimulationUIService;
        TickTraceUIService m_TickTraceUIService;
        TickDumpUIService m_TickDumpUIService;
        LogsUIService m_LogsUIService;
        BenchUIService m_BenchUIService;

    private:
        // Releases the old icons and opens the launcher if the setting asks for it.
        auto Initialize(const FrameContext& frame) -> void;

        // Releases the icons and drops the table snapshots.
        auto Shutdown() -> void;

        // Runs a pending datum capture, then draws the launcher and the visible tab windows.
        auto DrawFrame(const FrameContext& frame) -> void;
    };
}