export module Platform.Lifecycle.Hook:DestroySubsystems;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Lifecycle.State;
import Platform.Lifecycle.System;
import std;

export namespace Platform::Lifecycle::Hook
{
	class DestroySubsystemsDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using AOBService = Platform::Memory::System::AOBService;
		using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
		using LifecycleService = Platform::Lifecycle::System::LifecycleService;

	public:
		DestroySubsystemsDetour(LogsService& logsService, AOBService& aobService,
			LifecycleStore& lifecycleStore, LifecycleService& lifecycleService) :
			m_LogsService(logsService), m_AOBService(aobService),
			m_LifecycleStore(lifecycleStore), m_LifecycleService(lifecycleService) {}
		~DestroySubsystemsDetour() = default;

		auto Install() -> bool;
		auto Uninstall() -> void;

		auto GetFunctionAddress() const -> void*;

		static auto IsInProgress() -> bool;

	private:
		LogsService& m_LogsService;
		AOBService& m_AOBService;
		LifecycleStore& m_LifecycleStore;
		LifecycleService& m_LifecycleService;

		static DestroySubsystemsDetour* s_Instance;
		static inline std::atomic<bool> s_InProgress{ false };

		static auto __fastcall HookedDestroySubsystems() -> void;

		typedef auto(__fastcall* DestroySubsystems_t)() -> void;

		static inline DestroySubsystems_t m_OriginalFunction{ nullptr };
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}