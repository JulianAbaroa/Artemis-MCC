export module Viewer.Control.System;

import Service.Settings.State;
import Platform.Input.Type;
import Viewer.Camera.State;
import Viewer.Selection.State;
import Viewer.Overlay.State;
import std;

export namespace Viewer::Control::System
{
	class ControlService
	{
	private:
		using SettingsStore = Service::Settings::State::SettingsStore;
		using WindowMessage = Platform::Input::Type::WindowMessage;
		using RawMouse = Platform::Input::Type::RawMouse;
		using CameraStore = Viewer::Camera::State::CameraStore;
		using SelectionStore = Viewer::Selection::State::SelectionStore;
		using OverlayStore = Viewer::Overlay::State::OverlayStore;

	public:
		ControlService(SettingsStore& settingsStore, CameraStore& cameraStore, 
			SelectionStore& selectionStore, OverlayStore& overlayStore) :
			m_SettingsStore(settingsStore), m_CameraStore(cameraStore), 
			m_SelectionStore(selectionStore), m_OverlayStore(overlayStore) {}
		~ControlService() = default;

		ControlService(const ControlService&) = delete;
		ControlService& operator=(const ControlService&) = delete;

		auto HandleMessage(WindowMessage& message) -> bool;

		auto HandleMouse(RawMouse& mouse) -> bool;

	private:
		CameraStore& m_CameraStore;
		SelectionStore& m_SelectionStore;
		OverlayStore& m_OverlayStore;
		SettingsStore& m_SettingsStore;

		auto HandleHotkey(const WindowMessage& message) -> bool;
	};
}