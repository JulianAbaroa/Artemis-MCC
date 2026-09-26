export module Tables.Object.Hook:ReleaseObject;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Object.System;
import std;

export namespace Tables::Object::Hook
{
	class ReleaseObjectDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using ObjectTableService = Tables::Object::System::ObjectTableService;

	public:
		ReleaseObjectDetour(LogsService& logsService, AOBService& aobService, 
			ObjectTableService& objectService) : m_LogsService(logsService),
			m_AOBService(aobService), m_ObjectService(objectService) {};
		~ReleaseObjectDetour() = default;

		auto Install() -> void;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		ObjectTableService& m_ObjectService;

		static ReleaseObjectDetour* s_Instance;

		static auto __fastcall HookedReleaseObject(unsigned int handle) -> void;

		typedef auto(__fastcall* Release_t)(unsigned int handle) -> void;

		static inline Release_t m_OriginalFunction{ nullptr };
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}