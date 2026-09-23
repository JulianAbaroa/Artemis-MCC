export module UI.Backend.System:ImGui;

import Service.Logs.System;
import Service.Settings.State;
import Platform.Input.Type;
import Platform.Render.Type;
import Platform.Render.State;
import std;

export namespace UI::Backend::System
{
	class BackendUIService
	{
	private:
		using FrameContext = Platform::Render::Type::FrameContext;
		using WindowMessage = Platform::Input::Type::WindowMessage;

		using RenderStore = Platform::Render::State::RenderStore;
		using SettingsStore = Service::Settings::State::SettingsStore;
		using LogsService = Service::Logs::System::LogsService;

	public:
		BackendUIService(LogsService& logsService, SettingsStore& settingsStore,
			RenderStore& renderStore) : m_LogsService(logsService),
			m_SettingsStore(settingsStore), m_RenderStore(renderStore) {}
		~BackendUIService() = default;

		BackendUIService(const BackendUIService&) = delete;
		BackendUIService& operator=(const BackendUIService&) = delete;

		auto Initialize(const FrameContext& frame) -> void;
		auto OnResize(const FrameContext& frame) -> void;
		auto Shutdown() -> void;

		auto NewFrame(bool isMenuVisible) -> void;
		auto Render() -> void;

		auto ForwardMessage(WindowMessage& message) -> bool;
		auto WantsCapture(std::uint32_t message) const -> bool;

		auto IsInitialized() const -> bool;
		auto IsReady() const -> bool;

	private:
		LogsService& m_LogsService;
		SettingsStore& m_SettingsStore;
		RenderStore& m_RenderStore;

		std::atomic<bool> m_IsInitialized{ false };
		float m_AppliedScale{ 0.0f };

		auto ApplyPreferences() -> void;
		auto ApplyInputMode(bool isMenuVisible) -> void;
	};
}