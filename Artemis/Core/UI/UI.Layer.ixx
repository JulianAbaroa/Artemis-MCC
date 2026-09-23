export module UI.Layer;

import Service.Layer;
import Platform.Layer;
import Platform.Input.Type;
import Platform.Render.Type;
import Resolved.Layer;
import Export.Layer;
import Viewer.Layer;

import UI.Backend.System;
import UI.Hotkey.State;
import UI.Hotkey.System;
import UI.Launcher.State;
import UI.Launcher.System;
import UI.Overlay.State;
import UI.Overlay.System;
import UI.ObjectTable.State;
import UI.ObjectTable.System;
import UI.PlayerTable.State;
import UI.PlayerTable.System;
import UI.Settings.System;
import UI.MemoryScanner.State;
import UI.MemoryScanner.System;
import UI.Logs.State;
import UI.Logs.System;
import std;

export namespace UI
{
	class Layer
	{
	private:
		using FrameContext = Platform::Render::Type::FrameContext;
		using WindowMessage = Platform::Input::Type::WindowMessage;

		using HotkeyUIStore = UI::Hotkey::State::HotkeyUIStore;
		using LauncherUIStore = UI::Launcher::State::LauncherUIStore;
		using OverlayUIStore = UI::Overlay::State::OverlayUIStore;
		using ObjectTableUIStore = UI::ObjectTable::State::ObjectTableUIStore;
		using PlayerTableUIStore = UI::PlayerTable::State::PlayerTableUIStore;
		using MemoryScannerUIStore = UI::MemoryScanner::State::MemoryScannerUIStore;
		using LogsUIStore = UI::Logs::State::LogsUIStore;

		using BackendUIService = UI::Backend::System::BackendUIService;
		using WndProcUIService = UI::Backend::System::WndProcUIService;
		using HotkeyUIService = UI::Hotkey::System::HotkeyUIService;
		using LauncherUIService = UI::Launcher::System::LauncherUIService;
		using OverlayUIService = UI::Overlay::System::OverlayUIService;
		using ObjectTableUIService = UI::ObjectTable::System::ObjectTableUIService;
		using PlayerTableUIService = UI::PlayerTable::System::PlayerTableUIService;
		using SettingsUIService = UI::Settings::System::SettingsUIService;
		using MemoryScannerUIService = UI::MemoryScanner::System::MemoryScannerUIService;
		using LogsUIService = UI::Logs::System::LogsUIService;

	public:
		Layer(Service::Layer& service, Platform::Layer& platform, Resolved::Layer& resolved,
			Export::Layer& exportLayer, Viewer::Layer& viewer) :
			m_BackendUIService(service.m_LogsService, service.m_SettingsStore, platform.m_RenderStore),
			m_HotkeyUIService(service.m_SettingsStore, m_HotkeyUIStore, m_LauncherUIStore, m_OverlayUIStore),
			m_LauncherUIService(service.m_SettingsStore, m_LauncherUIStore),
			m_OverlayUIService(service.m_TelemetryStore, platform.m_RenderStore, resolved.m_VitalityStore, exportLayer.m_TickStore, viewer.m_OverlayStore, viewer.m_SelectionStore, m_OverlayUIStore),
			m_ObjectTableUIService(exportLayer.m_TickStore, m_ObjectTableUIStore),
			m_PlayerTableUIService(exportLayer.m_TickStore, m_PlayerTableUIStore),
			m_SettingsUIService(service.m_SettingsStore, service.m_SettingsService, service.m_LogsService, m_HotkeyUIStore),
			m_MemoryScannerUIService(platform.m_MemoryScannerStore, platform.m_MemoryScannerService, m_MemoryScannerUIStore),
			m_LogsUIService(service.m_SettingsStore, service.m_LogsStore, service.m_LogsService, m_LogsUIStore),
			m_WndProcUIService(m_BackendUIService, service.m_SettingsStore)
		{
			platform.m_RenderService.OnInitialized([this](const FrameContext& frame) {
				this->Initialize(frame);
			});

			platform.m_RenderService.OnResize([this](const FrameContext& frame) {
				m_BackendUIService.OnResize(frame);
			});

			platform.m_RenderService.OnFrame([this](const FrameContext& frame) {
				this->DrawFrame(frame);
			});

			platform.m_RenderService.OnShutdown([this] {
				this->Shutdown();
			});

			platform.m_InputService.OnWindowMessage([this](WindowMessage& message) {
				return m_HotkeyUIService.HandleMessage(message) || 
					m_WndProcUIService.HandleMessage(message);
			});
		}
		~Layer() = default;

		Layer(const Layer&) = delete;
		Layer& operator=(const Layer&) = delete;

		// --- State ---
		HotkeyUIStore m_HotkeyUIStore;
		LauncherUIStore m_LauncherUIStore;
		OverlayUIStore m_OverlayUIStore;
		ObjectTableUIStore m_ObjectTableUIStore;
		PlayerTableUIStore m_PlayerTableUIStore;
		MemoryScannerUIStore m_MemoryScannerUIStore;
		LogsUIStore m_LogsUIStore;

		// --- System ---
		BackendUIService m_BackendUIService;
		HotkeyUIService m_HotkeyUIService;
		LauncherUIService m_LauncherUIService;
		OverlayUIService m_OverlayUIService;
		ObjectTableUIService m_ObjectTableUIService;
		PlayerTableUIService m_PlayerTableUIService;
		SettingsUIService m_SettingsUIService;
		MemoryScannerUIService m_MemoryScannerUIService;
		LogsUIService m_LogsUIService;

		// --- Hook ---
		WndProcUIService m_WndProcUIService;

	private:
		auto Initialize(const FrameContext& frame) -> void;
		auto Shutdown() -> void;
		auto DrawFrame(const FrameContext& frame) -> void;
	};
}	