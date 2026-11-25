export module DevTools.Viewer.Layer;

import Common.Engine.Type;
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
import DevTools.Viewer.Director.State;
import DevTools.Viewer.Director.System;
import DevTools.Viewer.Selection.State;
import DevTools.Viewer.Selection.System;
import DevTools.Viewer.Latency.State;
import DevTools.Viewer.Latency.System;
import DevTools.Viewer.Overlay.State;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Control.System;
import DevTools.Viewer.Map.System;
import DevTools.Viewer.Hud.System;
import DevTools.Viewer.Inspector.System;
import DevTools.Viewer.Roster.System;
import DevTools.Viewer.Scene.System;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;

    using DevTools::Viewer::Director::System::DecidePovRequest;
    using DevTools::Viewer::Roster::System::DecidePossession;
}

export namespace DevTools::Viewer
{
    // Root of the Viewer layer. Owns the camera, the selection, the overlay options and the services that draw the scene and the HUD.
    // note: Hooks into the Platform events. It draws the 3D passes inside the game frame (the scene point), the labels and the HUD at Present, and handles the window and mouse input.
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
        using DirectorStore = DevTools::Viewer::Director::State::DirectorStore;
        using LatencyStore = DevTools::Viewer::Latency::State::LatencyStore;
        using OverlayStore = DevTools::Viewer::Overlay::State::OverlayStore;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;
        using Flag = DevTools::Viewer::Options::Type::Flag;
        using Scalar = DevTools::Viewer::Options::Type::Scalar;

        using CameraService = DevTools::Viewer::Camera::System::CameraService;
        using DirectorService = DevTools::Viewer::Director::System::DirectorService;
        using CollidableSceneService = DevTools::Viewer::Collidable::System::CollidableSceneService;
        using SelectionService = DevTools::Viewer::Selection::System::SelectionService;
        using ControlService = DevTools::Viewer::Control::System::ControlService;
        using SceneService = DevTools::Viewer::Scene::System::SceneService;
        using LatencyService = DevTools::Viewer::Latency::System::LatencyService;
        using MapService = DevTools::Viewer::Map::System::MapService;
        using HudService = DevTools::Viewer::Hud::System::HudService;
        using LabelService = DevTools::Viewer::Hud::System::LabelService;
        using AgentCardService = DevTools::Viewer::Hud::System::AgentCardService;
        using AgentStripService = DevTools::Viewer::Hud::System::AgentStripService;
        using InspectorService = DevTools::Viewer::Inspector::System::InspectorService;

    public:
        // param environment: Source of the debug collidables, which are not part of the Tick.
        // note: Draws the Tick that comes with the collidable scene, not the Tick store (D294).
        Layer(Services::Layer& service, DevTools::Viewer::Access::Type::EngineAccess& engine, Perception::Layer& environment,
            DevTools::Gui::Layer& gui,
            PreferencesService& preferences) :
            m_DirectorService(m_DirectorStore),
            m_CameraService(m_CameraStore, m_OptionsStore, engine.m_GameCameraService),
            m_CollidableSceneService(environment.m_CollidableService, m_CollidableStore),
            m_SelectionService(m_SelectionStore),
            m_ControlService(service.m_SettingsStore, m_CameraStore, m_SelectionStore, m_OptionsStore, m_DirectorStore),
            m_SceneService(service.m_SettingsStore, service.m_ObservedPlayerStore, m_CameraStore, m_CameraService, m_SelectionService,
                engine.m_RenderStore, engine.m_PovRequestStore, engine.m_PossessRequestStore, engine.m_BotStore, m_OptionsStore, m_DirectorStore, m_DirectorService),
            m_LatencyService(m_LatencyStore, engine.m_FrameClockReader, engine.m_CaptureStore),
            m_MapService(service.m_Log, m_CollidableStore, m_SceneService, m_OptionsStore, m_LatencyService,
                engine.m_LifecycleStore),
            m_HudService(service.m_TelemetryStore, engine.m_RenderStore, engine.m_PovStatusStore, engine.m_PossessionStore, engine.m_AgentClientStore, m_CollidableStore, m_OverlayStore, m_DirectorStore, m_SceneService,
                m_LatencyService),
            m_LabelService(m_CollidableStore, m_CameraService, m_SceneService, m_OptionsStore, m_SelectionStore),
            m_AgentCardService(m_CollidableStore, m_SceneService, m_OptionsStore, m_SelectionStore, engine.m_BotStore),
            m_AgentStripService(m_SceneService, m_OptionsStore, m_SelectionStore),
            m_InspectorService(m_SceneService, m_CollidableStore, m_SelectionStore, engine.m_PlayerActionStore, engine.m_BotStore)
        {
            auto& lifecycle = engine.m_LifecycleService;

            preferences.RegisterSection(std::string{ OptionsStore::k_PreferencePrefix },
                [this](std::ostream& stream) { m_OptionsStore.Save(stream); },
                [this](std::string_view key, std::string_view value) { m_OptionsStore.Load(key, value); });

            engine.m_RenderService.OnFrame([this, &gui, &engine, &service](const FrameContext&) {
                // The director plays for the map that is loaded. Its file is loaded when the map changes and saved after every edit, here, so the panel
                // works with the Viewer inactive too (VW.14).
                m_DirectorStore.SetMapName(engine.m_MapVariantService.GetActiveName());
                m_DirectorStore.SetMatchId(engine.m_MatchCycleStore.GetEngineStarts());
                m_DirectorService.Persist(service.m_SettingsStore.GetAppDataDirectory());

                // note: The HUD of a dead watched player is not hidden (D346): hiding it showed the first frames of the death text animation.
                engine.m_RenderStore.SetGameHudHidden(m_OptionsStore.IsEnabled(Flag::HideGameHud));
                engine.m_RenderStore.SetFirstPersonHidden(m_OptionsStore.IsEnabled(Flag::HideFirstPerson));

                // note: The overlay needs the depth copies of the first person mask, and the scene point draws only for an active Viewer. Without
                // the Viewer neither is done.
                const bool isActive{ m_SceneService.IsActive() };
                engine.m_RenderStore.SetFirstPersonMaskRequested(isActive && m_OptionsStore.IsEnabled(Flag::DrawOverlay));
                engine.m_RenderStore.SetSceneDrawRequested(isActive);

                // The game shows the observed player in the view of user 0 while the Viewer is active and the option is on. Without a request (the
                // option off, no player selected) the request is withdrawn so the view goes back to the local player in the next frame of the game.
                // An inactive Viewer withdraws it in Deactivate.
                if (isActive)
                {
                    const std::uint32_t observed{ service.m_ObservedPlayerStore.Get() };

                    // While the director plays the request is its own: a POV shot publishes its player and any other shot withdraws it, even with Watch POV
                    // on (VW.14f). The options are not changed: when the director stops, they rule again.
                    const std::uint32_t directed{ m_DirectorStore.GetPovPlayer() };

                    const std::optional<std::uint32_t> povRequest{ DecidePovRequest(m_DirectorStore.IsPlaying(), directed, m_OptionsStore.IsEnabled(Flag::WatchPov), observed) };
                    if (povRequest) engine.m_PovRequestStore.Publish(*povRequest);
                    else engine.m_PovRequestStore.Clear();

                    // note: The possession follows the selected player, not its body, so it survives the death of the bot (D352). A human, the Free
                    // camera or the option off withdraw it and the next tick of the game gives the bot back.
                    const std::optional<std::uint32_t> possessed{ DecidePossession(m_OptionsStore.IsEnabled(Flag::PossessBot), m_SceneService.IsGameMode(), observed, m_SceneService.IsBot(observed)) };
                    if (possessed) engine.m_PossessRequestStore.Publish(*possessed);
                    else engine.m_PossessRequestStore.Clear();
                }

                // The scene point does not run for an inactive Viewer, so the reset (override, flying state, scene, poses, observed player) is made
                // here. It is idempotent and on the same thread as the scene point.
                if (!isActive) m_SceneService.Deactivate();

                if (isActive && !m_IsActiveLast && !engine.m_RenderStore.IsOpaquePassHooked())
                {
                    service.m_Log.Message("[Viewer] ERROR: The hook of the opaque color pass (0x26E500) is not installed, so no 3D pass is drawn.");
                }

                m_IsActiveLast = isActive;

                // note: The store keeps the scale for the next scene draw.
                engine.m_RenderStore.SetSceneColorSlider(m_OptionsStore.GetScalar(Scalar::SceneColorScale));

                // The 3D passes are drawn at the scene point of the game frame (OnScene), not here. The labels use the scene of that update: a frame
                // without one (a menu, a pause without a draw) has none, and draws no labels.
                const bool isSceneUpdated{ m_SceneService.HasFrameUpdate() };
                m_SceneService.EndFrame();

                if (!gui.m_BackendGuiService.IsReady()) return;

                // The card and the strip have the gate of the labels: the same scene update, so never the data of another frame.
                if (isSceneUpdated)
                {
                    if (m_OptionsStore.IsEnabled(Flag::DrawOverlay)) m_LabelService.Draw();
                    m_AgentCardService.Draw();
                    m_AgentStripService.Draw();
                }

                m_HudService.Draw();
            });

            engine.m_RenderService.OnScene([this](const FrameContext& frame) {
                return m_MapService.RenderScene(frame);
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
        DirectorStore m_DirectorStore{};
        bool m_IsActiveLast{ false };

        // --- System ---
        DirectorService m_DirectorService;
        CameraService m_CameraService;
        CollidableSceneService m_CollidableSceneService;
        SelectionService m_SelectionService;
        ControlService m_ControlService;
        SceneService m_SceneService;
        LatencyService m_LatencyService;
        MapService m_MapService;
        HudService m_HudService;
        LabelService m_LabelService;
        AgentCardService m_AgentCardService;
        AgentStripService m_AgentStripService;
        InspectorService m_InspectorService;
    };
}