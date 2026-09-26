module;

#include <d3d11.h>

export module Viewer.Map.System:MapService;

import :Palette;
import :MapPass;
import :DynamicPass;
import :ZonePass;
import :RaycastPass;

import Service.Logs.System;
import Service.Settings.State;
import Platform.Render.System;
import Platform.Render.Type;
import Resolved.World.State;
import Export.Tick.State;
import Viewer.Camera.State;
import Viewer.Camera.System;
import Viewer.Selection.State;
import Viewer.Selection.System;
import std;

export namespace Viewer::Map::System
{
	class MapService
	{
	private:
		using FrameContext = Platform::Render::Type::FrameContext;

		using LogsService = Service::Logs::System::LogsService;
		using SettingsStore = Service::Settings::State::SettingsStore;
		using GpuPipeline = Platform::Render::System::GpuPipeline;
		using GpuStateGuard = Platform::Render::System::GpuStateGuard;
		using WorldStore = Resolved::World::State::WorldStore;
		using TickStore = Export::Tick::State::TickStore;
		using CameraStore = Viewer::Camera::State::CameraStore;
		using CameraService = Viewer::Camera::System::CameraService;
		using SelectionStore = Viewer::Selection::State::SelectionStore;
		using SelectionService = Viewer::Selection::System::SelectionService;

	public:
		MapService(LogsService& logsService, SettingsStore& settingsStore,
			WorldStore& worldStore, TickStore& tickStore,
			CameraStore& cameraStore, CameraService& cameraService,
			SelectionStore& selectionStore, SelectionService& selectionService) :
			m_WorldStore(worldStore), m_TickStore(tickStore),
			m_CameraStore(cameraStore), m_CameraService(cameraService),
			m_SelectionStore(selectionStore), m_SelectionService(selectionService),
			m_SettingsStore(settingsStore), m_LogsService(logsService),
			m_GpuPipeline(logsService), m_MapPass(logsService),
			m_DynamicPass(logsService), m_ZonePass(logsService),
			m_RaycastPass(logsService) {
		}
		~MapService() = default;

		MapService(const MapService&) = delete;
		MapService& operator=(const MapService&) = delete;

		auto Render(const FrameContext& frame) -> void;

		auto Release() -> void;
		auto Suspend() -> void;
		auto Resume() -> void;

	private:
		LogsService& m_LogsService;
		SettingsStore& m_SettingsStore;
		WorldStore& m_WorldStore;
		TickStore& m_TickStore;
		CameraStore& m_CameraStore;
		CameraService& m_CameraService;
		SelectionStore& m_SelectionStore;
		SelectionService& m_SelectionService;

		GpuPipeline m_GpuPipeline;
		MapPass m_MapPass;
		DynamicPass m_DynamicPass;
		ZonePass m_ZonePass;
		RaycastPass m_RaycastPass;
		PaletteService m_PaletteService{};

		std::mutex m_Mutex{};
		bool m_IsSuspended{ false };
		bool m_IsMapResetPending{ false };

		const ID3D11Device* m_Device{ nullptr };

		auto Draw(const FrameContext& frame) -> void;

		auto ReleaseMap() -> void;

		auto ReleaseAll() -> void;
	};
}