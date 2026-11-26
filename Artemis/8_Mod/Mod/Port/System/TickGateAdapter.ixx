export module Mod.Port.System:TickGate;

import Engine.Capture.Type;
import Bridge.Transport.System;
import std;

export namespace Mod::Port::System
{
    // Gives the tick gate the wait of the bridge between ticks (TG.1, D380): before each tick it asks BridgeService::HoldTick whether the
    // frame has to end there so the engine draws while Python answers.
    class TickGateAdapter : public Engine::Capture::Type::TickGate
    {
    private:
        using BridgeService = Bridge::Transport::System::BridgeService;

    public:
        explicit TickGateAdapter(BridgeService& bridgeService) : m_BridgeService(bridgeService) {}
        ~TickGateAdapter() override = default;

        TickGateAdapter(const TickGateAdapter&) = delete;
        auto operator=(const TickGateAdapter&) -> TickGateAdapter& = delete;

        auto IsTickHeld() -> bool override;

    private:
        BridgeService& m_BridgeService;
    };
}