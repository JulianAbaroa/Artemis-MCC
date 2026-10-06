export module UI.Layer;

import Service.Layer;
import Platform.Layer;
import Platform.Input.Type;
import Platform.Render.Type;
import Resolved.Layer;
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
import UI.MemoryScanner.State;
import UI.MemoryScanner.System;
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
        using FrameContext = Platform::Render::Type::FrameContext;
        using WindowMessage = Platform::Input::Type::WindowMessage;

        using BackendGuiService = Gui::Backend::System::BackendGuiService;

        using HotkeyUIStore = UI::Hotkey::State::HotkeyUIStore;
        using LauncherUIStore = UI::Launcher::State::LauncherUIStore;
        using ObjectTableUIStore = UI::ObjectTable::State::ObjectTableUIStore;
        using PlayerTableUIStore = UI::PlayerTable::State::PlayerTableUIStore;
        using MemoryScannerUIStore = UI::MemoryScanner::State::MemoryScannerUIStore;
        using LogsUIStore = UI::Logs::State::LogsUIStore;

        using HotkeyUIService = UI::Hotkey::System::HotkeyUIService;
        using LauncherUIService = UI::Launcher::System::LauncherUIService;
        using ObjectTableUIService = UI::ObjectTable::System::ObjectTableUIService;
        using PlayerTableUIService = UI::PlayerTable::System::PlayerTableUIService;
        using DefinitionsInspectorUIService = UI::DefinitionsInspector::System::DefinitionsInspectorUIService;
        using SettingsUIService = UI::Settings::System::SettingsUIService;
        using MemoryScannerUIService = UI::MemoryScanner::System::MemoryScannerUIService;
        using LogsUIService = UI::Logs::System::LogsUIService;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform, Resolved::Layer& resolved,
            Export::Layer& exportLayer, Gui::Layer& gui, Viewer::Layer& viewer) :
            m_BackendGuiService(gui.m_BackendGuiService),
            m_HotkeyUIService(service.m_SettingsStore, m_HotkeyUIStore, m_LauncherUIStore, viewer.m_OverlayStore),
            m_LauncherUIService(service.m_SettingsStore, m_LauncherUIStore),
            m_ObjectTableUIService(exportLayer.m_TickStore, m_ObjectTableUIStore),
            m_PlayerTableUIService(exportLayer.m_TickStore, m_PlayerTableUIStore),
            m_DefinitionsInspectorUIService(exportLayer.m_TickStore, resolved.m_DefinitionsStore, viewer.m_SelectionStore, service.m_SettingsStore),
            m_SettingsUIService(service.m_SettingsStore, service.m_SettingsService, service.m_LogsService, m_HotkeyUIStore, viewer.m_OptionsStore),
            m_MemoryScannerUIService(platform.m_MemoryScannerStore, platform.m_MemoryScannerService, m_MemoryScannerUIStore),
            m_LogsUIService(service.m_SettingsStore, service.m_LogsStore, m_LogsUIStore)
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

            platform.m_InputService.OnWindowMessage([this, &gui](WindowMessage& message) {
                return m_HotkeyUIService.HandleMessage(message) ||
                    gui.m_WndProcGuiService.HandleMessage(message);
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
        MemoryScannerUIStore m_MemoryScannerUIStore{};
        LogsUIStore m_LogsUIStore{};

        // --- System ---
        HotkeyUIService m_HotkeyUIService;
        LauncherUIService m_LauncherUIService;
        ObjectTableUIService m_ObjectTableUIService;
        PlayerTableUIService m_PlayerTableUIService;
        DefinitionsInspectorUIService m_DefinitionsInspectorUIService;
        SettingsUIService m_SettingsUIService;
        MemoryScannerUIService m_MemoryScannerUIService;
        LogsUIService m_LogsUIService;

    private:
        // Releases the old icons and opens the launcher if the setting asks for it.
        auto Initialize(const FrameContext& frame) -> void;

        // Releases the icons and drops the table snapshots.
        auto Shutdown() -> void;

        // Draws the launcher and the visible tab windows.
        auto DrawFrame(const FrameContext& frame) -> void;
    };
}