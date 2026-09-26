module;

#include <windows.h>

export module Platform.Input.Hook:GetRawInputData;

import Service.Logs.System;
import Service.Settings.State;
import Platform.Input.System;
import std;

export namespace Platform::Input::Hook
{
	class GetRawInputDataDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using SettingsStore = Service::Settings::State::SettingsStore;
		using InputService = Platform::Input::System::InputService;

	public:
		GetRawInputDataDetour(LogsService& logsService, SettingsStore& settingsStore,
			InputService& inputService) : m_LogsService(logsService),
			m_SettingsStore(settingsStore), m_InputService(inputService) {}
		~GetRawInputDataDetour() = default;

		auto Install() -> void;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		SettingsStore& m_SettingsStore;
		InputService& m_InputService;

		static GetRawInputDataDetour* s_Instance;

		static inline std::atomic<int> s_InFlight{ 0 };

		static auto WINAPI HookedGetRawInputData(HRAWINPUT hRawInput,
			UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)->UINT;

		typedef auto(WINAPI* GetRawInputData_t)(HRAWINPUT, UINT,
			LPVOID, PUINT, UINT)->UINT;

		static inline GetRawInputData_t m_OriginalFunction{ nullptr };
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}