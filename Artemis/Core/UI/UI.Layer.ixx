export module UI.Layer;

import Service.Layer;
import Service.Preferences.System;
import Platform.Layer;
import Platform.Input.Type;
import Platform.Render.Type;
import Map.Layer;
import Resolved.Layer;
import Datum.Layer;
import Tables.Layer;
import Export.Layer;
import Gui.Layer;
import Gui.Backend.System;
import Viewer.Layer;

import UI.Hotkey.State;
import UI.Hotkey.System;
import UI.Launcher.State;
import UI.Launcher.System;
import UI.ObjectTable.State;
import UI.ObjectTable.System;
import UI.PlayerTable.State;
import UI.PlayerTable.System;
import UI.DefinitionsInspector.System;
import UI.Settings.System;
import UI.Capture.State;
import UI.Capture.System;
import UI.Simulation.State;
import UI.Simulation.System;
import UI.TickTrace.State;
import UI.TickTrace.System;
import UI.TickDump.State;
import UI.TickDump.System;
import UI.Bench.State;
import UI.Bench.System;
import UI.Logs.State;
import UI.Logs.System;
import std;

export namespace UI
{
    // Root of the UI layer. Owns the stores and the services of the launcher and its tab windows.
    // note: Hooks into the Platform events. It draws the windows with the game frame and handles the hotkeys of the window messages.
    class Layer
    {
    private:
        using PreferencesService = Service::Preferences::System::PreferencesService;

        using FrameContext = Platform::Render::Type::FrameContext;
        using WindowMessage = Platform::Input::Type::WindowMessage;

        using BackendGuiService = Gui::Backend::System::BackendGuiService;

        using HotkeyUIStore = UI::Hotkey::State::HotkeyUIStore;
        using LauncherUIStore = UI::Launcher::State::LauncherUIStore;
        using ObjectTableUIStore = UI::ObjectTable::State::ObjectTableUIStore;
        using PlayerTableUIStore = UI::PlayerTable::State::PlayerTableUIStore;
        using CaptureUIStore = UI::Capture::State::CaptureUIStore;
        using SimulationUIStore = UI::Simulation::State::SimulationUIStore;
        using TickTraceUIStore = UI::TickTrace::State::TickTraceUIStore;
        using TickDumpUIStore = UI::TickDump::State::TickDumpUIStore;
        using BenchUIStore = UI::Bench::State::BenchUIStore;
        using LogsUIStore = UI::Logs::State::LogsUIStore;

        using HotkeyUIService = UI::Hotkey::System::HotkeyUIService;
        using LauncherUIService = UI::Launcher::System::LauncherUIService;
        using ObjectTableUIService = UI::ObjectTable::System::ObjectTableUIService;
        using PlayerTableUIService = UI::PlayerTable::System::PlayerTableUIService;
        using DefinitionsInspectorUIService = UI::DefinitionsInspector::System::DefinitionsInspectorUIService;
        using SettingsUIService = UI::Settings::System::SettingsUIService;
        using CaptureUIService = UI::Capture::System::CaptureUIService;
        using SimulationUIService = UI::Simulation::System::SimulationUIService;
        using TickTraceUIService = UI::TickTrace::System::TickTraceUIService;
        using TickDumpUIService = UI::TickDump::System::TickDumpUIService;
        using BenchUIService = UI::Bench::System::BenchUIService;
        using LogsUIService = UI::Logs::System::LogsUIService;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform, Map::Layer& map, Resolved::Layer& resolved,
            Datum::Layer& datum, Tables::Layer& tables, Export::Layer& exportLayer, Gui::Layer& gui, Viewer::Layer& viewer,
            PreferencesService& preferences) :
            m_BackendGuiService(gui.m_BackendGuiService),
            m_HotkeyUIService(service.m_SettingsStore, m_HotkeyUIStore, m_LauncherUIStore, m_CaptureUIStore, viewer.m_OverlayStore),
            m_LauncherUIService(service.m_SettingsStore, m_LauncherUIStore),
            m_ObjectTableUIService(exportLayer.m_TickStore, m_ObjectTableUIStore, m_CaptureUIStore),
            m_PlayerTableUIService(exportLayer.m_TickStore, m_PlayerTableUIStore),
            m_DefinitionsInspectorUIService(exportLayer.m_TickStore, resolved.m_DefinitionsStore, viewer.m_SelectionStore, service.m_SettingsStore),
            m_SettingsUIService(service.m_SettingsStore, service.m_SettingsService, service.m_LogsService, m_HotkeyUIStore, viewer.m_OptionsStore, preferences, viewer.m_CameraStore),
            m_CaptureUIService(service.m_LogsService, service.m_SettingsStore, platform.m_MemoryReaderService, resolved.m_DefinitionsStore, datum.m_DatumStore, exportLayer.m_TickStore, m_CaptureUIStore),
            m_SimulationUIService(service.m_LogsService, service.m_TelemetryStore, platform.m_LifecycleStore, platform.m_MatchConfigStore,
                platform.m_MatchConfigService, platform.m_LocalUserStore, platform.m_VirtualInputStore, platform.m_PlayerActionStore,
                platform.m_BotStore, platform.m_BridgeService, exportLayer.m_TickStore, m_SimulationUIStore),
            m_TickTraceUIService(service.m_LogsService, service.m_SettingsStore, map.m_FileStore, datum.m_DatumStore, tables.m_ObjectStore,
                m_TickTraceUIStore),
            m_TickDumpUIService(service.m_LogsService, service.m_SettingsStore, map.m_FileStore, exportLayer.m_FlatService,
                m_TickDumpUIStore),
            m_LogsUIService(service.m_SettingsStore, service.m_LogsStore, m_LogsUIStore),
            m_BenchUIService(service.m_LogsStore, service.m_LogsService, service.m_SettingsStore, platform.m_MemoryReaderService,
                platform.m_LocalUserStore, platform.m_PlayerActionStore, platform.m_VirtualInputStore, platform.m_LifecycleStore,
                platform.m_MatchConfigStore, platform.m_MatchCycleStore, platform.m_MatchConfigService, platform.m_BotStore,
                exportLayer.m_TickStore, m_TickTraceUIStore, m_BenchUIStore)
        {
            platform.m_RenderService.OnInitialized([this](const FrameContext& frame) {
                this->Initialize(frame);
            });

            platform.m_RenderService.OnFrame([this](const FrameContext& frame) {
                this->DrawFrame(frame);
            });

            platform.m_RenderService.OnShutdown([this] {
                this->Shutdown();
            });

            // The hotkeys run before every other handler, so the viewer cannot swallow them.
            platform.m_InputService.OnWindowMessage([this](WindowMessage& message) {
                return m_HotkeyUIService.HandleMessage(message);
            }, true);

            platform.m_InputService.OnWindowMessage([&gui](WindowMessage& message) {
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