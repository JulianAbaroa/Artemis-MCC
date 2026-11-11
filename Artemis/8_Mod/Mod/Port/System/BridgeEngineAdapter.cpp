module Mod.Port.System;
import :BridgeEngine;

import Engine.Base.Lifecycle.Type;
import Engine.Base.MapVariant.Type;
import Engine.Base.Preset.Type;
import std;

namespace
{
    using Engine::Base::Lifecycle::Type::Status;
    using Contract::Bridge::Type::BridgePhase;
    using Contract::Bridge::Type::BridgePresetResult;
    using Engine::Base::MapVariant::Type::MapVariantOutcome;
    using Engine::Base::Preset::Type::PresetOutcome;

    auto ToPresetResult(PresetOutcome outcome) -> BridgePresetResult
    {
        switch (outcome)
        {
        case PresetOutcome::None: return BridgePresetResult::NoResult;
        case PresetOutcome::Pending: return BridgePresetResult::Pending;
        case PresetOutcome::Applied: return BridgePresetResult::Applied;
        case PresetOutcome::AppliedAtLoad: return BridgePresetResult::AtLoad;
        case PresetOutcome::Rejected:
        case PresetOutcome::Failed: return BridgePresetResult::Rejected;
        default: return BridgePresetResult::Unknown;
        }
    }

    auto ToMapResult(MapVariantOutcome outcome) -> BridgePresetResult
    {
        switch (outcome)
        {
        case MapVariantOutcome::None: return BridgePresetResult::NoResult;
        case MapVariantOutcome::Pending: return BridgePresetResult::Pending;
        case MapVariantOutcome::Applied: return BridgePresetResult::Applied;
        case MapVariantOutcome::AppliedAtLoad: return BridgePresetResult::AtLoad;
        case MapVariantOutcome::Rejected:
        case MapVariantOutcome::Failed: return BridgePresetResult::Rejected;
        default: return BridgePresetResult::Unknown;
        }
    }

    auto ToPhase(Status status) -> BridgePhase
    {
        switch (status)
        {
        case Status::Running: return BridgePhase::Running;
        case Status::Initialized: return BridgePhase::Loading;
        default: return BridgePhase::Idle;
        }
    }
}

namespace Mod::Port::System
{
    auto BridgeEngineAdapter::GetPhase() const -> BridgePhase
    {
        return ToPhase(m_LifecycleStore.GetStatus());
    }

    auto BridgeEngineAdapter::GetEngineStarts() const -> std::uint64_t
    {
        return m_MatchCycleStore.GetEngineStarts();
    }

    auto BridgeEngineAdapter::GetPausedFrames() const -> std::uint64_t
    {
        return m_LifecycleStore.GetPausedFrames();
    }

    auto BridgeEngineAdapter::GetRequestedTimeScale() const -> float
    {
        return m_LifecycleStore.GetRequestedTimeScale();
    }

    auto BridgeEngineAdapter::SetRequestedTimeScale(float scale) -> void
    {
        m_LifecycleStore.SetRequestedTimeScale(scale);
    }

    auto BridgeEngineAdapter::IsPauseRequested() const -> bool
    {
        return m_LifecycleStore.IsPauseRequested();
    }

    auto BridgeEngineAdapter::SetPauseRequested(bool value) -> void
    {
        m_LifecycleStore.SetPauseRequested(value);
    }

    auto BridgeEngineAdapter::SetEndRoundRequested(bool value) -> void
    {
        m_LifecycleStore.SetEndRoundRequested(value);
    }

    auto BridgeEngineAdapter::RequestPreset(std::string_view name) -> void
    {
        // The match is where the ticks run, and it starts to be one while it loads. Anywhere else the preset is written at the next start.
        m_PresetService.Request(name, this->GetPhase() != BridgePhase::Idle);
    }

    auto BridgeEngineAdapter::CancelPresetRequest() -> void
    {
        m_PresetService.CancelPending();
    }

    auto BridgeEngineAdapter::GetPresetResult() const -> BridgePresetResult
    {
        return ToPresetResult(m_PresetStore.GetResult().Outcome);
    }

    auto BridgeEngineAdapter::GetActivePreset() const -> std::string
    {
        return m_PresetStore.GetActiveName();
    }

    auto BridgeEngineAdapter::RequestMap(std::string_view name) -> void
    {
        // The block is read here, on the bridge thread: the game thread only receives the bytes. No name asks for none.
        std::vector<std::byte> block{};
        if (!name.empty()) block = m_MatchConfigService.ReadMapVariant(name);

        // The match is where the ticks run, and it starts to be one while it loads: a request then waits for a break instead of being
        // written at a start that already happened. Anywhere else the map variant is written at the next start.
        m_MapVariantService.Request(name, std::move(block), this->GetPhase() != BridgePhase::Idle);
    }

    auto BridgeEngineAdapter::CancelMapRequest() -> void
    {
        m_MapVariantService.CancelPending();
    }

    auto BridgeEngineAdapter::GetMapResult() const -> BridgePresetResult
    {
        return ToMapResult(m_MapVariantStore.GetResult().Outcome);
    }

    auto BridgeEngineAdapter::GetActiveMap() const -> std::string
    {
        return m_MapVariantStore.GetActiveName();
    }

    auto BridgeEngineAdapter::SetOverride(std::size_t playerIndex, std::optional<ActionOverride> actionOverride) -> void
    {
        m_PlayerActionStore.SetOverride(playerIndex, actionOverride);
    }

    auto BridgeEngineAdapter::RequestClient(std::size_t playerIndex, const ClientRequests& requests) -> void
    {
        m_PlayerActionStore.RequestClient(playerIndex, requests);
    }
}