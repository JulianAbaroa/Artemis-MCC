export module Bridge.Transport.Layer;

import Services.Layer;
import Bridge.Transport.State;
import Bridge.Transport.System;

export namespace Bridge::Transport
{
    // The transport of the bridge: the shared memory with the Python client and the lockstep of the actions.
    // note: Owner only: it has no callbacks and installs nothing. It talks to the engine only through the BridgeEngine port (D215).
    class Layer
    {
    private:
        using BridgeEngine = Bridge::Transport::System::BridgeEngine;
        using BridgeStore = Bridge::Transport::State::BridgeStore;
        using BridgeService = Bridge::Transport::System::BridgeService;

    public:
        Layer(Services::Layer& service, BridgeEngine& engine) :
            m_BridgeService(service.m_Log, service.m_SettingsStore, engine, m_BridgeStore) {}
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        BridgeStore m_BridgeStore{};

        // --- System ---
        BridgeService m_BridgeService;
    };
}