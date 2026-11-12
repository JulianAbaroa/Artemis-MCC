module Mod.Port.System;
import :BridgeEngine;

import Engine.Base.Lifecycle.Type;
import Engine.Base.MatchSetup.Type;
import std;

namespace
{
    using Engine::Base::Lifecycle::Type::Status;
    using Contract::Bridge::Type::BridgePhase;
    using Contract::Bridge::Type::BridgePresetResult;
    using Engine::Base::MatchSetup::Type::MatchSetup;
    using Engine::Base::MatchSetup::Type::MatchSetupOrigin;
    using Engine::Base::MatchSetup::Type::MatchSetupOutcome;

    // The bridge has one result for preset and map: the one of the last setup request, whichever field it set (D286).
    auto ToSetupResult(MatchSetupOutcome outcome) -> BridgePresetResult
    {
        switch (outcome)
        {
        case MatchSetupOutcome::None: return BridgePresetResult::NoResult;
        case MatchSetupOutcome::Pending: return BridgePresetResult::Pending;
        case MatchSetupOutcome::Applied: return BridgePresetResult::Applied;
        case MatchSetupOutcome::AtLoad: return BridgePresetResult::AtLoad;
        case MatchSetupOutcome::Rejected: return BridgePresetResult::Rejected;
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
        // The files are read here, on the bridge thread: the game thread only receives the values.
        auto pick{ m_PresetService.Resolve(name) };
        if (!pick)
        {
            m_MatchSetupService.Reject(pick.error());
            return;
        }

        // The bridge asks for the preset and the map in two calls: the second adds to the setup the first left waiting.
        m_MatchSetupService.Request(MatchSetup{ .Preset = std::move(*pick) }, MatchSetupOrigin::Bridge, true);
    }

    auto BridgeEngineAdapter::CancelPresetRequest() -> void
    {
        m_MatchSetupService.Cancel(MatchSetupOrigin::Bridge);
    }

    auto BridgeEngineAdapter::GetPresetResult() const -> BridgePresetResult
    {
        return ToSetupResult(m_MatchSetupService.GetResult().Outcome);
    }

    auto BridgeEngineAdapter::GetActivePreset() const -> std::string
    {
        return m_MatchSetupService.GetActive().Preset;
    }

    auto BridgeEngineAdapter::RequestMap(std::string_view name) -> void
    {
        // The block is read here, on the bridge thread: the game thread only receives the bytes. No name asks for none.
        std::vector<std::byte> block{};
        if (!name.empty()) block = m_MatchConfigService.ReadMapVariant(name);

        auto mapVariant{ m_MapVariantService.Resolve(name, std::move(block)) };
        if (!mapVariant)
        {
            m_MatchSetupService.Reject(mapVariant.error());
            return;
        }

        m_MatchSetupService.Request(MatchSetup{ .Map = std::move(*mapVariant) }, MatchSetupOrigin::Bridge, true);
    }

    auto BridgeEngineAdapter::CancelMapRequest() -> void
    {
        m_MatchSetupService.Cancel(MatchSetupOrigin::Bridge);
    }

    auto BridgeEngineAdapter::GetMapResult() const -> BridgePresetResult
    {
        return ToSetupResult(m_MatchSetupService.GetResult().Outcome);
    }

    auto BridgeEngineAdapter::GetActiveMap() const -> std::string
    {
        return m_MatchSetupService.GetActive().Map;
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