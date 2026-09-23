export module UI.Overlay.System;

export import :Affordances;
export import :Collidables;
export import :Fixtures;
export import :Vitalities;

import Service.Telemetry.State;
import Platform.Render.State;
import Resolved.Vitality.State;
import Export.Tick.Type;
import Export.Tick.State;
import Viewer.Overlay.State;
import Viewer.Selection.State;
import UI.Overlay.State;
import std;

export namespace UI::Overlay::System
{
	class OverlayUIService
	{
	private:
		using Tick = Export::Tick::Type::Tick;

		using TelemetryStore = Service::Telemetry::State::TelemetryStore;
		using RenderStore = Platform::Render::State::RenderStore;
		using VitalityStore = Resolved::Vitality::State::VitalityStore;
		using TickStore = Export::Tick::State::TickStore;
		using OverlayStore = Viewer::Overlay::State::OverlayStore;
		using SelectionStore = Viewer::Selection::State::SelectionStore;
		using OverlayUIStore = UI::Overlay::State::OverlayUIStore;

	public:
		OverlayUIService(TelemetryStore& telemetryStore, RenderStore& renderStore,
			VitalityStore& vitalityStore, TickStore& tickStore, OverlayStore& overlayStore,
			SelectionStore& selectionStore, OverlayUIStore& overlayVisibilityStore) :
			m_TelemetryStore(telemetryStore), m_RenderStore(renderStore),
			m_VitalityStore(vitalityStore), m_TickStore(tickStore),
			m_OverlayStore(overlayStore), m_SelectionStore(selectionStore),
			m_OverlayVisibilityStore(overlayVisibilityStore) {
		}
		~OverlayUIService() = default;

		OverlayUIService(const OverlayUIService&) = delete;
		OverlayUIService& operator=(const OverlayUIService&) = delete;

		auto Draw() -> void;

	private:
		TelemetryStore& m_TelemetryStore;
		RenderStore& m_RenderStore;
		VitalityStore& m_VitalityStore;
		TickStore& m_TickStore;
		OverlayStore& m_OverlayStore;
		SelectionStore& m_SelectionStore;
		OverlayUIStore& m_OverlayVisibilityStore;

		std::uint32_t m_LastHandle{ 0xFFFFFFFF };

		auto DrawNavBar() -> void;
		auto DrawDefault() -> void;
		auto DrawSelectedPanel(const std::shared_ptr<const Tick>& tick) -> void;
		auto DrawPanel(const Tick& tick, std::uint32_t handle) -> void;

		auto DrawFramerate() -> void;
		auto DrawTelemetry() -> void;
	};
}