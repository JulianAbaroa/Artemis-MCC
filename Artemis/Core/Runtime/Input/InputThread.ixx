export module Runtime.Thread:Input;

import Service.Layer;
import Platform.Layer;
import std;

export namespace Runtime::Thread
{
	class InputThread
	{
	public:
		InputThread(Service::Layer& service, Platform::Layer& platform) :
			m_Service(service), m_Platform(platform) {}
		~InputThread() = default;

		auto Run() -> void;

	private:
		Service::Layer& m_Service;
		Platform::Layer& m_Platform;
	};
}