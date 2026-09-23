export module Runtime.Thread:Main;

import Service.Layer;
import Platform.Layer;
import std;

export namespace Runtime::Thread
{
	class MainThread
	{
	public:
		MainThread(Service::Layer& service, Platform::Layer& platform) :
			m_Service(service), m_Platform(platform) {}
		~MainThread() = default;

		auto Run() -> void;

	private:
		Service::Layer& m_Service;
		Platform::Layer& m_Platform;

		auto WaitOrExit(std::chrono::milliseconds ms) -> bool;

		auto InstallLifecycleHooks() -> bool;
		auto InstallRenderHooks() -> bool;
		auto Shutdown() -> void;

		auto CheckHooksHealth() -> void;
		auto IsHookIntact(void* address) -> bool;

		auto IsStillRunning() -> bool;
	};
}