export module DevTools.Viewer.Hud.System;

export import :Affordances;
export import :Collidables;
export import :Fixtures;
export import :Vitalities;
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
import Engine.Render.State;
import Bridge.Codec.Tick.Type;
import Bridge.Codec.Tick.State;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Overlay.State;
import DevTools.Viewer.Selection.State;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace DevTools::Viewer::Hud::System
{
    // Draws the overlay window with the telemetry or the panel of the selected object.
    class HudService
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;

        using TelemetryStore = Services::Telemetry::State::TelemetryStore;
        using RenderStore = Engine::Render::State::RenderStore;
        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;
        using TickStore = Bridge::Codec::Tick::State::TickStore;
        using OverlayStore = DevTools::Viewer::Overlay::State::OverlayStore;
        using SelectionStore = DevTools::Viewer::Selection::State::SelectionStore;

    public:
        HudService(TelemetryStore& telemetryStore, RenderStore& renderStore, CollidableStore& collidableStore,
            TickStore& tickStore, OverlayStore& overlayStore,
            SelectionStore& selectionStore) :
            m_TelemetryStore(telemetryStore), m_RenderStore(renderStore), m_CollidableStore(collidableStore),
            m_TickStore(tickStore),
            m_OverlayStore(overlayStore), m_SelectionStore(selectionStore) {}
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
        TickStore& m_TickStore;
        OverlayStore& m_OverlayStore;
        SelectionStore& m_SelectionStore;

        std::uint32_t m_LastHandle{ k_NoHandle };

        auto DrawNavBar() -> void;
        auto DrawDefault() -> void;
        auto DrawSelectedPanel(const std::shared_ptr<const Tick>& tick) -> void;
        auto DrawPanel(const Tick& tick, std::uint32_t handle) -> void;

        auto DrawFramerate() -> void;
        auto DrawTelemetry() -> void;
    };
}