export module Platform.Lifecycle.System;

import Service.Logs.System;
import Platform.Lifecycle.State;
import std;

export namespace Platform::Lifecycle::System
{
	class LifecycleService
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;

	public:
		using Callback = std::function<void()>;

		LifecycleService(LogsService& logsService, LifecycleStore& lifecycleStore) :
			m_LogsService(logsService), m_LifecycleStore(lifecycleStore) {}
		~LifecycleService() = default;

		auto SignalShutdown() -> void;

		auto OnEngineInitialized(Callback callback) -> void;
		auto OnUnhook(Callback callback) -> void;
		auto OnCleanup(Callback callback) -> void;
		auto OnShutdown(Callback callback) -> void;

		auto RaiseEngineInitialized() -> void;
		auto RaiseUnhook() -> void;
		auto RaiseCleanup() -> void;
		auto RaiseShutdown() -> void;

	private:
		LogsService& m_LogsService;
		LifecycleStore& m_LifecycleStore;

		std::vector<Callback> m_OnEngineInitialized{};
		std::vector<Callback> m_OnUnhook{};
		std::vector<Callback> m_OnCleanup{};
		std::vector<Callback> m_OnShutdown{};

		std::atomic<bool> m_IsShutdownRaised{ false };
	};
}