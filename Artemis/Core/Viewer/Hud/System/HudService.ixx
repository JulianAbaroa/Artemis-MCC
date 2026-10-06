export module Viewer.Hud.System;

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

import Service.Telemetry.State;
import Platform.Render.State;
import Export.Tick.Type;
import Export.Tick.State;
import Viewer.Overlay.State;
import Viewer.Selection.State;
import std;

namespace
{
    using Viewer::Selection::State::k_NoSelection;
}

export namespace Viewer::Hud::System
{
    // Draws the overlay window with the telemetry or the panel of the selected object.
    class HudService
    {
    private:
        using Tick = Export::Tick::Type::Tick;

        using TelemetryStore = Service::Telemetry::State::TelemetryStore;
        using RenderStore = Platform::Render::State::RenderStore;
        using TickStore = Export::Tick::State::TickStore;
        using OverlayStore = Viewer::Overlay::State::OverlayStore;
        using SelectionStore = Viewer::Selection::State::SelectionStore;

    public:
        HudService(TelemetryStore& telemetryStore, RenderStore& renderStore,
            TickStore& tickStore, OverlayStore& overlayStore,
            SelectionStore& selectionStore) :
            m_TelemetryStore(telemetryStore), m_RenderStore(renderStore),
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
        TickStore& m_TickStore;
        OverlayStore& m_OverlayStore;
        SelectionStore& m_SelectionStore;

        std::uint32_t m_LastHandle{ k_NoSelection };

        auto DrawNavBar() -> void;
        auto DrawDefault() -> void;
        auto DrawSelectedPanel(const std::shared_ptr<const Tick>& tick) -> void;
        auto DrawPanel(const Tick& tick, std::uint32_t handle) -> void;

        auto DrawFramerate() -> void;
        auto DrawTelemetry() -> void;
    };
}