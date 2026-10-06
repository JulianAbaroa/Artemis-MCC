export module Viewer.Layer;

import Service.Layer;
import Service.Preferences.System;
import Platform.Layer;
import Platform.Render.Type;
import Platform.Input.Type;
import Export.Layer;
import Gui.Layer;
import Gui.Backend.System;
import Viewer.Camera.State;
import Viewer.Camera.System;
import Viewer.Selection.State;
import Viewer.Selection.System;
import Viewer.Overlay.State;
import Viewer.Options.State;
import Viewer.Control.System;
import Viewer.Map.System;
import Viewer.Hud.System;
import Viewer.Scene.System;
import std;

export namespace Viewer
{
    // Root of the Viewer layer. Owns the camera, the selection, the overlay options and the services that draw the scene and the HUD.
    // note: Hooks into the Platform events. It renders the map and the HUD with the game frame and handles the window and mouse input.
    class Layer
    {
    private:
        using PreferencesService = Service::Preferences::System::PreferencesService;

        using FrameContext = Platform::Render::Type::FrameContext;
        using WindowMessage = Platform::Input::Type::WindowMessage;
        using RawMouse = Platform::Input::Type::RawMouse;

        using CameraStore = Viewer::Camera::State::CameraStore;
        using SelectionStore = Viewer::Selection::State::SelectionStore;
        using OverlayStore = Viewer::Overlay::State::OverlayStore;
        using OptionsStore = Viewer::Options::State::OptionsStore;

        using CameraService = Viewer::Camera::System::CameraService;
        using SelectionService = Viewer::Selection::System::SelectionService;
        using ControlService = Viewer::Control::System::ControlService;
        using SceneService = Viewer::Scene::System::SceneService;
        using MapService = Viewer::Map::System::MapService;
        using HudService = Viewer::Hud::System::HudService;
        using LabelService = Viewer::Hud::System::LabelService;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform,
            Export::Layer& exportLayer, Gui::Layer& gui,
            PreferencesService& preferences) :
            m_CameraService(m_CameraStore),
            m_SelectionService(m_SelectionStore),
            m_ControlService(service.m_SettingsStore, m_CameraStore, m_SelectionStore, m_OverlayStore, m_OptionsStore),
            m_SceneService(service.m_SettingsStore, m_CameraStore, m_CameraService, m_SelectionService),
            m_MapService(service.m_LogsService, exportLayer.m_TickStore, m_SceneService, m_OptionsStore),
            m_HudService(service.m_TelemetryStore, platform.m_RenderStore, exportLayer.m_TickStore, m_OverlayStore, m_SelectionStore),
            m_LabelService(exportLayer.m_TickStore, m_CameraService, m_SceneService, m_OptionsStore, m_SelectionStore)
        {
            auto& lifecycle = platform.m_LifecycleService;

            preferences.RegisterSection(std::string{ OptionsStore::k_PreferencePrefix },
                [this](std::ostream& stream) { m_OptionsStore.Save(stream); },
                [this](std::string_view key, std::string_view value) { m_OptionsStore.Load(key, value); });

            platform.m_RenderService.OnFrame([this, &gui](const FrameContext& frame) {
                m_MapService.Render(frame);

                if (!gui.m_BackendGuiService.IsReady()) return;

                m_LabelService.Draw();
                m_HudService.Draw();
            });

            platform.m_RenderService.OnShutdown([this] {
                m_MapService.Release();
            });

            platform.m_InputService.OnWindowMessage([this](WindowMessage& message) {
                return m_ControlService.HandleMessage(message);
            });

            platform.m_InputService.OnRawMouse([this](RawMouse& mouse) {
                return m_ControlService.HandleMouse(mouse);
            });

            lifecycle.OnUnhook([this] {
                m_MapService.Suspend();
            });

            lifecycle.OnEngineInitialized([this] {
                m_MapService.Resume();
            });
        }
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        CameraStore m_CameraStore{};
        SelectionStore m_SelectionStore{};
        OverlayStore m_OverlayStore{};
        OptionsStore m_OptionsStore{};

        // --- System ---
        CameraService m_CameraService;
        SelectionService m_SelectionService;
        ControlService m_ControlService;
        SceneService m_SceneService;
        MapService m_MapService;
        HudService m_HudService;
        LabelService m_LabelService;
    };
}