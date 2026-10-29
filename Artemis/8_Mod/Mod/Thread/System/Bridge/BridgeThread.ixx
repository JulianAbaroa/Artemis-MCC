export module Mod.Thread.System:Bridge;

import Services.Layer;
import Engine.Base.Layer;
import Bridge.Transport.Layer;
import Bridge.Codec.Layer;
import std;

export namespace Mod::Thread::System
{
    // Thread of the bridge with Python (D135): opens the shared memory, publishes each decision tick the AI thread hands
    // over and keeps the control block up to date. Closes the shared memory when the lifecycle stops.
    // note: Without a client nothing is flattened: the AI thread only hands over ticks while a client is connected.
    class BridgeThread
    {
    public:
        BridgeThread(Services::Layer& service, Engine::Base::Layer& base, Bridge::Transport::Layer& transport,
            Bridge::Codec::Layer& exportLayer) :
            m_Service(service), m_Base(base), m_Transport(transport), m_Export(exportLayer) {}
        ~BridgeThread() = default;

        // Blocks until the lifecycle stops.
        auto Run() -> void;

    private:
        Services::Layer& m_Service;
        Engine::Base::Layer& m_Base;
        Bridge::Transport::Layer& m_Transport;
        Bridge::Codec::Layer& m_Export;

        // Flattens and publishes one decision tick.
        auto Publish() -> void;
    };
}