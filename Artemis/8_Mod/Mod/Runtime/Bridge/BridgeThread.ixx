export module Runtime.Thread:Bridge;

import Service.Layer;
import Platform.Layer;
import Export.Layer;
import std;

export namespace Runtime::Thread
{
    // Thread of the bridge with Python (D135): opens the shared memory, publishes each decision tick the AI thread hands
    // over and keeps the control block up to date. Closes the shared memory when the lifecycle stops.
    // note: Without a client nothing is flattened: the AI thread only hands over ticks while a client is connected.
    class BridgeThread
    {
    public:
        BridgeThread(Service::Layer& service, Platform::Layer& platform, Export::Layer& exportLayer) :
            m_Service(service), m_Platform(platform), m_Export(exportLayer) {}
        ~BridgeThread() = default;

        // Blocks until the lifecycle stops.
        auto Run() -> void;

    private:
        Service::Layer& m_Service;
        Platform::Layer& m_Platform;
        Export::Layer& m_Export;

        // Flattens and publishes one decision tick.
        auto Publish() -> void;
    };
}