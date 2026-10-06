export module Runtime.Thread:Input;

import Service.Layer;
import Platform.Layer;
import std;

export namespace Runtime::Thread
{
    // Thread that stays alive while the lifecycle runs.
    // note: Input is gathered by the hooks, so it only logs its start and stop.
    class InputThread
    {
    public:
        InputThread(Service::Layer& service, Platform::Layer& platform) :
            m_Service(service), m_Platform(platform) {}
        ~InputThread() = default;

        // Blocks until the lifecycle stops.
        auto Run() -> void;

    private:
        Service::Layer& m_Service;
        Platform::Layer& m_Platform;
    };
}