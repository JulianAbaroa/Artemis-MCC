export module Mod.Port.System:ActionSource;

import Engine.Control.Input.System;
import Bridge.Transport.System;
import std;

export namespace Mod::Port::System
{
    // Gives the PlayerAction hook the actions of the client of the bridge (D215): Prepare applies them through the bridge.
    class BridgeActionSource : public Engine::Control::Input::System::ActionSource
    {
    private:
        using BridgeService = Bridge::Transport::System::BridgeService;

    public:
        explicit BridgeActionSource(BridgeService& bridgeService) : m_BridgeService(bridgeService) {}
        ~BridgeActionSource() override = default;

        BridgeActionSource(const BridgeActionSource&) = delete;
        auto operator=(const BridgeActionSource&) -> BridgeActionSource& = delete;

        auto Prepare(std::uint64_t actionTick) -> void override;

    private:
        BridgeService& m_BridgeService;
    };
}