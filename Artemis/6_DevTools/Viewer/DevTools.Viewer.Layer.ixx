export module DevTools.Viewer.Layer;

import Services.Layer;
import Services.Observed.State;
import Services.Preferences.System;
import Engine.Render.Type;
import Engine.Render.Window.Type;
import Perception.Layer;
import DevTools.Viewer.Access.Type;
import DevTools.Gui.Layer;
import DevTools.Gui.Backend.System;
import DevTools.Viewer.Camera.State;
import DevTools.Viewer.Camera.System;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Collidable.System;
import DevTools.Viewer.Selection.State;
import DevTools.Viewer.Selection.System;
import DevTools.Viewer.Latency.State;
import DevTools.Viewer.Latency.System;
import DevTools.Viewer.Recorder.System;
import DevTools.Viewer.Overlay.State;
import DevTools.Viewer.Options.Type;
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
        using LatencyStore = DevTools::Viewer::Latency::State::LatencyStore;
        using OverlayStore = DevTools::Viewer::Overlay::State::OverlayStore;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;
        using Flag = DevTools::Viewer::Options::Type::Flag;

        using CameraService = DevTools::Viewer::Camera::System::CameraService;
        using CollidableSceneService = DevTools::Viewer::Collidable::System::CollidableSceneService;
        using SelectionService = DevTools::Viewer::Selection::System::SelectionService;
        using ControlService = DevTools::Viewer::Control::System::ControlService;
        using SceneService = DevTools::Viewer::Scene::System::SceneService;
        using LatencyService = DevTools::Viewer::Latency::System::LatencyService;
        using FrameClockRecorder = DevTools::Viewer::Recorder::System::FrameClockRecorder;
        using MapService = DevTools::Viewer::Map::System::MapService;
        using HudService = DevTools::Viewer::Hud::System::HudService;
        using LabelService = DevTools::Viewer::Hud::System::LabelService;

    public:
        // param environment: Source of the debug collidables, which are not part of the Tick.
        // note: Draws the Tick that comes with the collidable scene, not the Tick store (D294).
        Layer(Services::Layer& service, DevTools::Viewer::Access::Type::EngineAccess& engine, Perception::Layer& environment,
            DevTools::Gui::Layer& gui,
            PreferencesService& preferences) :
            m_CameraService(m_CameraStore, m_OptionsStore, engine.m_GameCameraService),
            m_CollidableSceneService(environment.m_CollidableService, m_CollidableStore),
            m_SelectionService(m_SelectionStore),
            m_ControlService(service.m_SettingsStore, m_CameraStore, m_SelectionStore, m_OverlayStore, m_OptionsStore),
            m_SceneService(service.m_SettingsStore, service.m_ObservedPlayerStore, m_CameraStore, m_CameraService, m_SelectionService,
                engine.m_RenderStore),
            m_LatencyService(m_LatencyStore, engine.m_FrameClockReader, engine.m_CaptureStore),
            m_FrameClockRecorder(service.m_Log, service.m_SettingsStore),
            m_MapService(service.m_Log, m_CollidableStore, m_SceneService, m_OptionsStore, m_LatencyService, m_FrameClockRecorder,
                engine.m_LifecycleStore),
            m_HudService(service.m_TelemetryStore, engine.m_RenderStore, m_CollidableStore, m_OverlayStore, m_SelectionStore, m_SceneService,
                m_LatencyService),
            m_LabelService(m_CollidableStore, m_CameraService, m_SceneService, m_OptionsStore, m_SelectionStore)
        {
            auto& lifecycle = engine.m_LifecycleService;

            preferences.RegisterSection(std::string{ OptionsStore::k_PreferencePrefix },
                [this](std::ostream& stream) { m_OptionsStore.Save(stream); },
                [this](std::string_view key, std::string_view value) { m_OptionsStore.Load(key, value); });

            engine.m_RenderService.OnFrame([this, &gui, &engine](const FrameContext& frame) {
                engine.m_RenderStore.SetGameHudHidden(m_OptionsStore.IsEnabled(Flag::HideGameHud));
                engine.m_RenderStore.SetFirstPersonHidden(m_OptionsStore.IsEnabled(Flag::HideFirstPerson));

                // note: The mask only matters under the overlay, so the depth copies are not made without it.
                engine.m_RenderStore.SetFirstPersonMaskRequested(m_OptionsStore.IsEnabled(Flag::FirstPersonOverOverlay) &&
                    m_OptionsStore.IsEnabled(Flag::Overlay));

                // note: Drawing in the scene also drops the color mask pair, which that point cannot use (VW.6.z).
                engine.m_RenderStore.SetOverlayInSceneRequested(m_OptionsStore.IsEnabled(Flag::OverlayInScene) &&
                    m_OptionsStore.IsEnabled(Flag::Overlay));

                // The 3D passes ran under the HUD of the game when the event marked the frame. Otherwise they run here, once.
                if (!engine.m_RenderStore.IsBeforeHudMarked()) m_MapService.Render(frame);

                if (!gui.m_BackendGuiService.IsReady()) return;

                m_LabelService.Draw();
                m_HudService.Draw();
            });

            engine.m_RenderService.OnBeforeHud([this](const FrameContext& frame) {
                return m_MapService.RenderBeforeHud(frame);
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
        LatencyStore m_LatencyStore{};

        // --- System ---
        CameraService m_CameraService;
        CollidableSceneService m_CollidableSceneService;
        SelectionService m_SelectionService;
        ControlService m_ControlService;
        SceneService m_SceneService;
        LatencyService m_LatencyService;
        FrameClockRecorder m_FrameClockRecorder;
        MapService m_MapService;
        HudService m_HudService;
        LabelService m_LabelService;
    };
}