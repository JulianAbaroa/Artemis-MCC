export module Tables.Object.Hook:InitRootNode;

import Service.Logs.System;
import Platform.Memory.System;
import Tables.Object.State;
import std;

export namespace Tables::Object::Hook
{
	class InitRootNodeDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using BoneOffsetsStore = Tables::Object::State::BoneOffsetsStore;

	public:
		InitRootNodeDetour(LogsService& logsService, AOBService& aobService,
			BoneOffsetsStore& boneOffsetsStore) : m_LogsService(logsService),
			m_AOBService(aobService), m_BoneOffsetsStore(boneOffsetsStore) {}
		~InitRootNodeDetour() = default;

		auto Install() -> void;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		BoneOffsetsStore& m_BoneOffsetsStore;

		static InitRootNodeDetour* s_Instance;

		static auto __fastcall HookedInitRootNode(unsigned short* param_1) -> void;

		typedef auto(__fastcall* InitRootNode_t)(unsigned short* param_1) -> void;

		static inline InitRootNode_t m_OriginalFunction = nullptr;
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}