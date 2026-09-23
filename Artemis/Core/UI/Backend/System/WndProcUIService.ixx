export module UI.Backend.System:WndProc;

import :ImGui;
import Platform.Input.Type;
import Service.Settings.State;

export namespace UI::Backend::System
{
	class WndProcUIService
	{
	private:
		using WindowMessage = Platform::Input::Type::WindowMessage;

		using SettingsStore = Service::Settings::State::SettingsStore;
		using BackendUIService = UI::Backend::System::BackendUIService;

	public:
		WndProcUIService(BackendUIService& backend, SettingsStore& settingsStore) :
			m_Backend(backend), m_SettingsStore(settingsStore) {}
		~WndProcUIService() = default;

		WndProcUIService(const WndProcUIService&) = delete;
		WndProcUIService& operator=(const WndProcUIService&) = delete;

		auto HandleMessage(WindowMessage& message) -> bool;

	private:
		BackendUIService& m_Backend;
		SettingsStore& m_SettingsStore;
	};
}