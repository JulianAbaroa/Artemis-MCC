export module DevTools.Viewer.Hud.System;

export import :Canvas;
export import :PlayerLabels;
export import :ObjectLabels;
export import :VehicleLabels;
export import :HealthLabels;
export import :InteractionLabels;
export import :FixtureLabels;
export import :Labels;
export import :AgentCard;
export import :AgentStrip;

import Services.Telemetry.State;
import Engine.Base.Pov.State;
import Engine.Control.Input.State;
import Engine.Render.Type;
import Engine.Render.State;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Latency.System;
import DevTools.Viewer.Overlay.State;
import DevTools.Viewer.Scene.System;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Draws the overlay window with the telemetry.
    class HudService
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;

        using TelemetryStore = Services::Telemetry::State::TelemetryStore;

        using PovStatusStore = Engine::Base::Pov::State::PovStatusStore;
        using PossessionStore = Engine::Control::Input::State::PossessionStore;
        using RenderStore = Engine::Render::State::RenderStore;
        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;
        using OverlayStore = DevTools::Viewer::Overlay::State::OverlayStore;
        using SceneService = DevTools::Viewer::Scene::System::SceneService;
        using LatencyService = DevTools::Viewer::Latency::System::LatencyService;

    public:
        HudService(TelemetryStore& telemetryStore, RenderStore& renderStore, const PovStatusStore& povStatusStore, const PossessionStore& possessionStore,
            CollidableStore& collidableStore,
            OverlayStore& overlayStore, SceneService& sceneService, LatencyService& latencyService) :
            m_TelemetryStore(telemetryStore), m_RenderStore(renderStore), m_PovStatusStore(povStatusStore), m_PossessionStore(possessionStore), m_CollidableStore(collidableStore),
            m_OverlayStore(overlayStore), m_SceneService(sceneService), m_LatencyService(latencyService) {}
        ~HudService() = default;

        HudService(const HudService&) = delete;
        auto operator=(const HudService&) -> HudService& = delete;

        // Draws the overlay window.
        // note: Does nothing if the overlay is hidden.
        auto Draw() -> void;

    private:
        TelemetryStore& m_TelemetryStore;
        RenderStore& m_RenderStore;
        const PovStatusStore& m_PovStatusStore;
        const PossessionStore& m_PossessionStore;
        CollidableStore& m_CollidableStore;
        OverlayStore& m_OverlayStore;
        SceneService& m_SceneService;
        LatencyService& m_LatencyService;

        auto DrawDefault() -> void;

        auto DrawFramerate() -> void;
        auto DrawTelemetry() -> void;

        // Draws the camera of the game: validity, position, fields of view, aspect against the back buffer and the distance to the View of the
        // local player. It answers whether the overlay lines up (D323).
        auto DrawGameCamera(const std::shared_ptr<const Tick>& tick) -> void;
        auto DrawFirstPersonHook() -> void;

        // Draws the POV: whether the view assignment of the engine was found, and while a player is requested who it is, the handles of the
        // request, of the view of the game and of the local player, and the calls, reasserts and faults (VW.9a, D340).
        auto DrawPov(const std::shared_ptr<const Tick>& tick) -> void;

        // Draws the possession of a bot: off, or the player and the ticks the client of the user drove it, the ones with an action and the ones
        // that left a branch unported, and that the unit is missing (VW.9c.2 B).
        auto DrawPossession(const std::shared_ptr<const Tick>& tick) -> void;

        // Draws whether the first person mask of the overlay was copied in the last frame (VW.6.y.2 B).
        auto DrawFirstPersonMask() -> void;

        // Draws the measurement of the lag between the scene of the overlay and the frame of the game (VW.5c), over the last k_WindowFrames.
        auto DrawLatency() -> void;

        // Draws where the overlay was drawn in the last frame: the scene point, or none with the reason.
        auto DrawOverlayPoint() -> void;

        // Draws the scale of the last scene draw.
        auto DrawSceneColor() -> void;
        auto DrawOverride() -> void;
    };
}