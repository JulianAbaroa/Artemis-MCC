export module Bridge.Transport.Layer;

import Service.Layer;
import Platform.Bridge.State;
import Platform.Bridge.System;

export namespace Bridge::Transport
{
    // The transport of the bridge: the shared memory with the Python client and the lockstep of the actions.
    // note: Owner only: it has no callbacks and installs nothing. It talks to the engine only through the BridgeEngine port (D215).
    class Layer
    {
    private:
        using BridgeEngine = Platform::Bridge::System::BridgeEngine;
        using BridgeStore = Platform::Bridge::State::BridgeStore;
        using BridgeService = Platform::Bridge::System::BridgeService;

    public:
        Layer(Service::Layer& service, BridgeEngine& engine) :
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