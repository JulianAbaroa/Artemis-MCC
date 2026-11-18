export module DevTools.Viewer.Hud.System;

export import :Affordances;
export import :Collidables;
export import :Fixtures;
export import :Vitalities;
export import :ObservedView;
export import :Canvas;
export import :PlayerLabels;
export import :ObjectLabels;
export import :VehicleLabels;
export import :HealthLabels;
export import :InteractionLabels;
export import :FixtureLabels;
export import :Labels;

import Common.Engine.Type;
import Services.Telemetry.State;
import Engine.Render.Type;
import Engine.Render.State;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Latency.System;
import DevTools.Viewer.Overlay.State;
import DevTools.Viewer.Scene.System;
import DevTools.Viewer.Selection.State;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace DevTools::Viewer::Hud::System
{
    // Draws the overlay window with the telemetry, the panel of the selected object or the View of the watched player.
    class HudService
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;

        using TelemetryStore = Services::Telemetry::State::TelemetryStore;
        using BeforeHudStatus = Engine::Render::Type::BeforeHudStatus;
        using HudHookInfo = Engine::Render::Type::HudHookInfo;

        using RenderStore = Engine::Render::State::RenderStore;
        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;
        using OverlayStore = DevTools::Viewer::Overlay::State::OverlayStore;
        using SelectionStore = DevTools::Viewer::Selection::State::SelectionStore;
        using SceneService = DevTools::Viewer::Scene::System::SceneService;
        using LatencyService = DevTools::Viewer::Latency::System::LatencyService;

    public:
        HudService(TelemetryStore& telemetryStore, RenderStore& renderStore, CollidableStore& collidableStore,
            OverlayStore& overlayStore, SelectionStore& selectionStore, SceneService& sceneService, LatencyService& latencyService) :
            m_TelemetryStore(telemetryStore), m_RenderStore(renderStore), m_CollidableStore(collidableStore),
            m_OverlayStore(overlayStore), m_SelectionStore(selectionStore), m_SceneService(sceneService), m_LatencyService(latencyService) {}
        ~HudService() = default;

        HudService(const HudService&) = delete;
        auto operator=(const HudService&) -> HudService& = delete;

        // Draws the overlay window.
        // note: Does nothing if the overlay is hidden.
        auto Draw() -> void;

    private:
        TelemetryStore& m_TelemetryStore;
        RenderStore& m_RenderStore;
        CollidableStore& m_CollidableStore;
        OverlayStore& m_OverlayStore;
        SelectionStore& m_SelectionStore;
        SceneService& m_SceneService;
        LatencyService& m_LatencyService;

        std::uint32_t m_LastHandle{ k_NoHandle };

        auto DrawNavBar() -> void;
        auto DrawDefault() -> void;
        auto DrawViewPanel(const std::shared_ptr<const Tick>& tick) -> void;
        auto DrawSelectedPanel(const std::shared_ptr<const Tick>& tick) -> void;
        auto DrawPanel(const Tick& tick, std::uint32_t handle) -> void;

        auto DrawFramerate() -> void;
        auto DrawTelemetry() -> void;

        // Draws the camera of the game: validity, position, fields of view, aspect against the back buffer and the distance to the View of the
        // local player. It answers whether the overlay lines up (D323).
        auto DrawGameCamera(const std::shared_ptr<const Tick>& tick) -> void;
        auto DrawHudHook() -> void;
        auto DrawFirstPersonHook() -> void;

        // Draws the measurement of the lag between the scene of the overlay and the frame of the game (VW.5c), over the last k_WindowFrames.
        auto DrawLatency() -> void;
        auto DrawOverlayPoint(bool isHooked) -> void;
        auto DrawOverride() -> void;
    };
}