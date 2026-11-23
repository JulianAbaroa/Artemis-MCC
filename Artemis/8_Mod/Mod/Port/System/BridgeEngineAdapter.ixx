export module Mod.Port.System:BridgeEngine;

import Control.Client.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.MapVariant.System;
import Engine.Base.MatchSetup.System;
import Engine.Base.Preset.System;
import Engine.Match.State;
import Engine.Match.System;
import Engine.Control.Input.State;
import Engine.Control.Input.System;
import Bridge.Transport.Type;
import Bridge.Transport.System;
import std;

export namespace Mod::Port::System
{
    // Gives the bridge the engine state it reads and writes (D215): the lifecycle, the match cycle and the player actions.
    class BridgeEngineAdapter : public Bridge::Transport::System::BridgeEngine
    {
    private:
        using AgentInput = Control::Client::Type::AgentInput;

        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using MapVariantService = Engine::Base::MapVariant::System::MapVariantService;
        using MatchSetupService = Engine::Base::MatchSetup::System::MatchSetupService;
        using PresetService = Engine::Base::Preset::System::PresetService;
        using MatchCycleStore = Engine::Match::State::MatchCycleStore;
        using MatchConfigService = Engine::Match::System::MatchConfigService;

        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;
        using PossessionStore = Engine::Control::Input::State::PossessionStore;
        using PossessionService = Engine::Control::Input::System::PossessionService;

        using BridgePhase = Contract::Bridge::Type::BridgePhase;
        using BridgeActiveSetup = Bridge::Transport::Type::BridgeActiveSetup;
        using BridgeImitation = Bridge::Transport::Type::BridgeImitation;
        using BridgeSetupInfo = Bridge::Transport::Type::BridgeSetupInfo;
        using BridgeSetupRequest = Bridge::Transport::Type::BridgeSetupRequest;

    public:
        BridgeEngineAdapter(LifecycleStore& lifecycleStore, MatchCycleStore& matchCycleStore, PlayerActionStore& playerActionStore,
            PossessionStore& possessionStore, PossessionService& possessionService, MatchSetupService& matchSetupService, PresetService& presetService,
            MapVariantService& mapVariantService, MatchConfigService& matchConfigService) :
            m_LifecycleStore(lifecycleStore), m_MatchCycleStore(matchCycleStore), m_PlayerActionStore(playerActionStore),
            m_PossessionStore(possessionStore), m_PossessionService(possessionService), m_MatchSetupService(matchSetupService), m_PresetService(presetService), m_MapVariantService(mapVariantService),
            m_MatchConfigService(matchConfigService) {}
        ~BridgeEngineAdapter() override = default;

        BridgeEngineAdapter(const BridgeEngineAdapter&) = delete;
        auto operator=(const BridgeEngineAdapter&) -> BridgeEngineAdapter& = delete;

        auto GetPhase() const -> BridgePhase override;
        auto GetEngineStarts() const -> std::uint64_t override;
        auto GetPausedFrames() const -> std::uint64_t override;
        auto GetRequestedTimeScale() const -> float override;
        auto SetRequestedTimeScale(float scale) -> void override;
        auto IsPauseRequested() const -> bool override;
        auto SetPauseRequested(bool value) -> void override;
        auto SetEndRoundRequested(bool value) -> void override;
        auto RequestSetup(const BridgeSetupRequest& request) -> void override;
        auto CancelSetupRequest() -> void override;
        auto GetSetupResult() const -> BridgeSetupInfo override;
        auto GetActiveSetup() const -> BridgeActiveSetup override;
        auto SetAgentInput(std::size_t playerIndex, std::uint32_t playerHandle, const AgentInput& input, std::uint32_t windowTicks,
            float turnLimit) -> void override;
        auto ClearAgentInput(std::size_t playerIndex) -> void override;
        auto CloseImitationWindow() -> void override;
        auto GetImitation() const -> BridgeImitation override;

    private:
        LifecycleStore& m_LifecycleStore;
        MatchCycleStore& m_MatchCycleStore;
        PlayerActionStore& m_PlayerActionStore;
        PossessionStore& m_PossessionStore;
        PossessionService& m_PossessionService;
        MatchSetupService& m_MatchSetupService;
        PresetService& m_PresetService;
        MapVariantService& m_MapVariantService;
        MatchConfigService& m_MatchConfigService;
    };
}