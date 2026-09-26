export module Platform.Lifecycle.Hook:EngineInitialize;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Lifecycle.State;
import Platform.Lifecycle.System;
import std;

export namespace Platform::Lifecycle::Hook
{
	class EngineInitializeDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
		using LifecycleService = Platform::Lifecycle::System::LifecycleService;

	public:
		EngineInitializeDetour(LogsService& logsService, AOBService& aobService,
			LifecycleStore& lifecycleStore, LifecycleService& lifecycleService) :
			m_LogsService(logsService), m_AOBService(aobService),
			m_LifecycleStore(lifecycleStore), m_LifecycleService(lifecycleService) {}
		~EngineInitializeDetour() = default;

		auto Install() -> bool;
		auto Uninstall() -> void;

		auto GetFunctionAddress() const -> void*;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		LifecycleStore& m_LifecycleStore;
		LifecycleService& m_LifecycleService;

		static EngineInitializeDetour* s_Instance;

		static auto __fastcall HookedEngineInitialize() -> void;

		typedef auto(__fastcall* EngineInitialize_t)() -> void;

		static inline EngineInitialize_t m_OriginalFunction{ nullptr };
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}