export module DevTools.Viewer.Layer;

import Services.Layer;
import Services.Preferences.System;
import Engine.Render.Type;
import Engine.Render.Window.Type;
import Perception.Layer;
import Bridge.Codec.Layer;
import DevTools.Viewer.Access.Type;
import DevTools.Gui.Layer;
import DevTools.Gui.Backend.System;
import DevTools.Viewer.Camera.State;
import DevTools.Viewer.Camera.System;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Collidable.System;
import DevTools.Viewer.Selection.State;
import DevTools.Viewer.Selection.System;
import DevTools.Viewer.Overlay.State;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Control.System;
import DevTools.Viewer.Map.System;
import DevTools.Viewer.Hud.System;
import DevTools.Viewer.Scene.System;
import std;

export namespace DevTools::Viewer
{
    // Root of the Viewer layer. Owns the camera, the selection, the overlay options and the services that draw the scene and the HUD.
    // note: Hooks into the Platform events. It renders the map and the HUD with the game frame and handles the window and mouse input.
    class Layer
    {
    private:
        using PreferencesService = Services::Preferences::System::PreferencesService;

        using FrameContext = Engine::Render::Type::FrameContext;
        using WindowMessage = Engine::Render::Window::Type::WindowMessage;
        using RawMouse = Engine::Render::Window::Type::RawMouse;

        using CameraStore = DevTools::Viewer::Camera::State::CameraStore;
        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;
        using SelectionStore = DevTools::Viewer::Selection::State::SelectionStore;
        using OverlayStore = DevTools::Viewer::Overlay::State::OverlayStore;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;

        using CameraService = DevTools::Viewer::Camera::System::CameraService;
        using CollidableSceneService = DevTools::Viewer::Collidable::System::CollidableSceneService;
        using SelectionService = DevTools::Viewer::Selection::System::SelectionService;
        using ControlService = DevTools::Viewer::Control::System::ControlService;
        using SceneService = DevTools::Viewer::Scene::System::SceneService;
        using MapService = DevTools::Viewer::Map::System::MapService;
        using HudService = DevTools::Viewer::Hud::System::HudService;
        using LabelService = DevTools::Viewer::Hud::System::LabelService;

    public:
        // param environment: Source of the debug collidables, which are not part of the Tick.
        Layer(Services::Layer& service, DevTools::Viewer::Access::Type::EngineAccess& engine, Perception::Layer& environment,
            Bridge::Codec::Layer& exportLayer, DevTools::Gui::Layer& gui,
            PreferencesService& preferences) :
            m_CameraService(m_CameraStore, m_OptionsStore),
            m_CollidableSceneService(environment.m_CollidableService, m_CollidableStore),
            m_SelectionService(m_CollidableStore, m_SelectionStore),
            m_ControlService(service.m_SettingsStore, m_CameraStore, m_SelectionStore, m_OverlayStore, m_OptionsStore),
            m_SceneService(service.m_SettingsStore, m_CameraStore, m_CameraService, m_SelectionService),
            m_MapService(service.m_Log, m_CollidableStore, exportLayer.m_TickStore, m_SceneService, m_OptionsStore),
            m_HudService(service.m_TelemetryStore, engine.m_RenderStore, m_CollidableStore, exportLayer.m_TickStore, m_OverlayStore, m_SelectionStore),
            m_LabelService(exportLayer.m_TickStore, m_CameraService, m_SceneService, m_OptionsStore, m_SelectionStore)
        {
            auto& lifecycle = engine.m_LifecycleService;

            preferences.RegisterSection(std::string{ OptionsStore::k_PreferencePrefix },
                [this](std::ostream& stream) { m_OptionsStore.Save(stream); },
                [this](std::string_view key, std::string_view value) { m_OptionsStore.Load(key, value); });

            engine.m_RenderService.OnFrame([this, &gui](const FrameContext& frame) {
                m_MapService.Render(frame);

                if (!gui.m_BackendGuiService.IsReady()) return;

                m_LabelService.Draw();
                m_HudService.Draw();
            });

            engine.m_RenderService.OnShutdown([this] {
                m_MapService.Release();
            });

            engine.m_InputService.OnWindowMessage([this](WindowMessage& message) {
                return m_ControlService.HandleMessage(message);
            });

            engine.m_InputService.OnRawMouse([this](RawMouse& mouse) {
                return m_ControlService.HandleMouse(mouse);
            });

            lifecycle.OnCleanup([this] {
                m_CollidableSceneService.Cleanup();
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
        CollidableStore m_CollidableStore{};
        SelectionStore m_SelectionStore{};
        OverlayStore m_OverlayStore{};
        OptionsStore m_OptionsStore{};

        // --- System ---
        CameraService m_CameraService;
        CollidableSceneService m_CollidableSceneService;
        SelectionService m_SelectionService;
        ControlService m_ControlService;
        SceneService m_SceneService;
        MapService m_MapService;
        HudService m_HudService;
        LabelService m_LabelService;
    };
}