export module Platform.Port.System:ActionSource;

import Platform.Input.System;
import Platform.Bridge.System;
import std;

export namespace Platform::Port::System
{
    // Gives the PlayerAction hook the actions of the client of the bridge (D215): Prepare applies them through the bridge.
    class BridgeActionSource : public Platform::Input::System::ActionSource
    {
    private:
        using BridgeService = Platform::Bridge::System::BridgeService;

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