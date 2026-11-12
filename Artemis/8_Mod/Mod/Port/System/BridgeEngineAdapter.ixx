export module Mod.Port.System:BridgeEngine;

import Control.Action.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.MapVariant.System;
import Engine.Base.MatchSetup.System;
import Engine.Base.Preset.System;
import Engine.Match.State;
import Engine.Match.System;
import Engine.Control.Input.State;
import Bridge.Transport.Type;
import Bridge.Transport.System;
import std;

export namespace Mod::Port::System
{
    // Gives the bridge the engine state it reads and writes (D215): the lifecycle, the match cycle and the player actions.
    class BridgeEngineAdapter : public Bridge::Transport::System::BridgeEngine
    {
    private:
        using ActionOverride = Control::Action::Type::ActionOverride;
        using ClientRequests = Control::Action::Type::ClientRequests;

        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using MapVariantService = Engine::Base::MapVariant::System::MapVariantService;
        using MatchSetupService = Engine::Base::MatchSetup::System::MatchSetupService;
        using PresetService = Engine::Base::Preset::System::PresetService;
        using MatchCycleStore = Engine::Match::State::MatchCycleStore;
        using MatchConfigService = Engine::Match::System::MatchConfigService;

        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;

        using BridgePhase = Contract::Bridge::Type::BridgePhase;
        using BridgeActiveSetup = Bridge::Transport::Type::BridgeActiveSetup;
        using BridgeSetupInfo = Bridge::Transport::Type::BridgeSetupInfo;
        using BridgeSetupRequest = Bridge::Transport::Type::BridgeSetupRequest;

    public:
        BridgeEngineAdapter(LifecycleStore& lifecycleStore, MatchCycleStore& matchCycleStore, PlayerActionStore& playerActionStore,
            MatchSetupService& matchSetupService, PresetService& presetService, MapVariantService& mapVariantService,
            MatchConfigService& matchConfigService) :
            m_LifecycleStore(lifecycleStore), m_MatchCycleStore(matchCycleStore), m_PlayerActionStore(playerActionStore),
            m_MatchSetupService(matchSetupService), m_PresetService(presetService), m_MapVariantService(mapVariantService),
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
        auto SetOverride(std::size_t playerIndex, std::optional<ActionOverride> actionOverride) -> void override;
        auto RequestClient(std::size_t playerIndex, const ClientRequests& requests) -> void override;

    private:
        LifecycleStore& m_LifecycleStore;
        MatchCycleStore& m_MatchCycleStore;
        PlayerActionStore& m_PlayerActionStore;
        MatchSetupService& m_MatchSetupService;
        PresetService& m_PresetService;
        MapVariantService& m_MapVariantService;
        MatchConfigService& m_MatchConfigService;
    };
}