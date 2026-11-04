export module Mod.Port.System:BridgeEngine;

import Control.Action.Type;
import Engine.Base.Lifecycle.State;
import Engine.Base.Preset.State;
import Engine.Base.Preset.System;
import Engine.Match.State;
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
        using PresetStore = Engine::Base::Preset::State::PresetStore;
        using PresetService = Engine::Base::Preset::System::PresetService;
        using MatchCycleStore = Engine::Match::State::MatchCycleStore;

        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;

        using BridgePhase = Contract::Bridge::Type::BridgePhase;
        using BridgePresetResult = Contract::Bridge::Type::BridgePresetResult;

    public:
        BridgeEngineAdapter(LifecycleStore& lifecycleStore, MatchCycleStore& matchCycleStore, PlayerActionStore& playerActionStore,
            PresetStore& presetStore, PresetService& presetService) :
            m_LifecycleStore(lifecycleStore), m_MatchCycleStore(matchCycleStore), m_PlayerActionStore(playerActionStore),
            m_PresetStore(presetStore), m_PresetService(presetService) {}
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
        auto RequestPreset(std::string_view name) -> void override;
        auto CancelPresetRequest() -> void override;
        auto GetPresetResult() const -> BridgePresetResult override;
        auto GetActivePreset() const -> std::string override;
        auto SetOverride(std::size_t playerIndex, std::optional<ActionOverride> actionOverride) -> void override;
        auto RequestClient(std::size_t playerIndex, const ClientRequests& requests) -> void override;

    private:
        LifecycleStore& m_LifecycleStore;
        MatchCycleStore& m_MatchCycleStore;
        PlayerActionStore& m_PlayerActionStore;
        PresetStore& m_PresetStore;
        PresetService& m_PresetService;
    };
}