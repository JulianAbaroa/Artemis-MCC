export module Platform.Input.Hook:GetButtonState;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Input.State;
import std;

export namespace Platform::Input::Hook
{
	class GetButtonStateDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using InputStore = Platform::Input::State::InputStore;

	public:
		GetButtonStateDetour(LogsService& logsService, AOBService& aobService, 
			InputStore& inputStore) : m_LogsService(logsService), 
			m_AOBService(aobService), m_InputStore(inputStore) {}
		~GetButtonStateDetour() = default;

		auto Install() -> void;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		InputStore& m_InputStore;

		static GetButtonStateDetour* s_Instance;

		static auto __fastcall HookedGetButtonState(short buttonID) -> char;

		typedef auto(__fastcall* GetButtonState_t)(short buttonID) -> char;

		static inline GetButtonState_t m_OriginalFunction{ nullptr };
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}