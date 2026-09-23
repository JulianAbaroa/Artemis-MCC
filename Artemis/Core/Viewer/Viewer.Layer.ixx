export module Viewer.Layer;

import Service.Layer;
import Platform.Layer;
import Resolved.Layer;
import Export.Layer;
import Platform.Render.Type;
import Platform.Input.Type;
import Viewer.Camera.State;
import Viewer.Camera.System;
import Viewer.Selection.State;
import Viewer.Selection.System;
import Viewer.Overlay.State;
import Viewer.Control.System;
import Viewer.Map.System;
import std;

export namespace Viewer
{
	class Layer
	{
	private:
		using CameraStore = Viewer::Camera::State::CameraStore;
		using SelectionStore = Viewer::Selection::State::SelectionStore;
		using OverlayStore = Viewer::Overlay::State::OverlayStore;

		using CameraService = Viewer::Camera::System::CameraService;
		using SelectionService = Viewer::Selection::System::SelectionService;
		using ControlService = Viewer::Control::System::ControlService;
		using MapService = Viewer::Map::System::MapService;

		using FrameContext = Platform::Render::Type::FrameContext;
		using WindowMessage = Platform::Input::Type::WindowMessage;
		using RawMouse = Platform::Input::Type::RawMouse;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform,
			Resolved::Layer& resolved, Export::Layer& exportLayer) :
			m_CameraService(m_CameraStore, m_SelectionStore),
			m_SelectionService(m_SelectionStore),
			m_ControlService(service.m_SettingsStore, m_CameraStore, m_SelectionStore, m_OverlayStore),
			m_MapService(service.m_LogsService, service.m_SettingsStore, resolved.m_WorldStore, exportLayer.m_TickStore, m_CameraStore, m_CameraService, m_SelectionStore, m_SelectionService)
		{
			auto& lifecycle = platform.m_LifecycleService;

			platform.m_RenderService.OnFrame([this](const FrameContext& frame) {
				m_MapService.Render(frame);
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

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		CameraStore m_CameraStore;
		SelectionStore m_SelectionStore;
		OverlayStore m_OverlayStore;

		// --- System ---
		CameraService m_CameraService;
		SelectionService m_SelectionService;
		ControlService m_ControlService;
		MapService m_MapService;
	};
}