export module Tables.Object.Hook:CreateObject;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Object.System;
import std;

export namespace Tables::Object::Hook
{
	class CreateObjectDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using ObjectTableService = Tables::Object::System::ObjectTableService;

	public:
		CreateObjectDetour(LogsService& logsService, AOBService& aobService,  
			ObjectTableService& objectService) : m_LogsService(logsService),
			m_AOBService(aobService), m_ObjectService(objectService) {}
		~CreateObjectDetour() = default;

		auto Install() -> void;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		ObjectTableService& m_ObjectService;

		static CreateObjectDetour* s_Instance;

		static auto __fastcall HookedCreateObject(
			unsigned short* placementData) -> unsigned long long;

		typedef auto(__fastcall* Create_t)(
			unsigned short* placementData) -> unsigned long long;

		static inline Create_t m_OriginalFunction{ nullptr };
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}