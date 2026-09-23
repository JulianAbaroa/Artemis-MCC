export module UI.Settings.System;

import Service.Settings.State;
import Service.Settings.System;
import Service.Logs.System;
import UI.Hotkey.State;
import std;

export namespace UI::Settings::System
{
	class SettingsUIService
	{
	private:
		using SettingsStore = Service::Settings::State::SettingsStore;
		using SettingsService = Service::Settings::System::SettingsService;
		using LogsService = Service::Logs::System::LogsService;
		using HotkeyUIStore = UI::Hotkey::State::HotkeyUIStore;

		static constexpr float k_AnimationDuration{ 0.6f };

	public:
		SettingsUIService(SettingsStore& settingsStore, SettingsService& settingsService,
			LogsService& logsService, HotkeyUIStore& hotkeyStore) :
			m_SettingsStore(settingsStore), m_SettingsService(settingsService),
			m_LogsService(logsService), m_HotkeyStore(hotkeyStore) {
		}
		~SettingsUIService() = default;

		SettingsUIService(const SettingsUIService&) = delete;
		SettingsUIService& operator=(const SettingsUIService&) = delete;

		auto Draw() -> void;

	private:
		SettingsStore& m_SettingsStore;
		SettingsService& m_SettingsService;
		LogsService& m_LogsService;
		HotkeyUIStore& m_HotkeyStore;

		std::string m_AnimatedPathLabel{};
		float m_AnimationStartTime{ 0.0f };

		float m_UIScalePreview{ 1.0f };
		bool m_IsScalePreviewInitialized{ false };

		auto DrawUserPreferences() -> void;
		auto DrawHotkeysTable() -> void;
		auto DrawDataPersistence() -> void;
		auto DrawSystemDirectories() -> void;

		auto DrawHotkeyRow(const char* label, const char* keys, const char* tooltip) -> void;
		auto DrawPathField(const char* label, const std::string& path,
			float widthOffset = 10.0f) -> void;

		auto DrawPersistencePopups() -> void;
		auto DrawConfirmDisableAppData() -> void;
		auto DrawDeleteAllAppData() -> void;
	};
}