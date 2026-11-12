module Mod.Port.System;
import :BridgeEngine;

import Engine.Base.Lifecycle.Type;
import Engine.Base.MatchSetup.Type;
import std;

namespace
{
    using Engine::Base::Lifecycle::Type::Status;
    using Contract::Bridge::Type::BridgePhase;
    using Contract::Bridge::Type::BridgeRoster;
    using Contract::Bridge::Type::BridgeSetupResult;
    using Contract::Bridge::Type::k_BridgeSetupReasonSize;
    using Engine::Base::MatchSetup::Type::MatchSetup;
    using Engine::Base::MatchSetup::Type::MatchSetupOrigin;
    using Engine::Base::MatchSetup::Type::MatchSetupOutcome;
    using Engine::Base::MatchSetup::Type::Roster;

    // The bridge has one result for the preset, the map and the roster of a setup request (D286).
    auto ToSetupResult(MatchSetupOutcome outcome) -> BridgeSetupResult
    {
        switch (outcome)
        {
        case MatchSetupOutcome::None: return BridgeSetupResult::NoResult;
        case MatchSetupOutcome::Pending: return BridgeSetupResult::Pending;
        case MatchSetupOutcome::Applied: return BridgeSetupResult::Applied;
        case MatchSetupOutcome::AtLoad: return BridgeSetupResult::AtLoad;
        case MatchSetupOutcome::Rejected: return BridgeSetupResult::Rejected;
        default: return BridgeSetupResult::Unknown;
        }
    }

    // return: The roster of the bridge (bots as bytes) as the engine one. A count is never above the bot limit, which fits a byte.
    auto ToRoster(const BridgeRoster& roster) -> Roster
    {
        Roster result{};
        for (std::size_t team{ 0 }; team < roster.Teams.size(); ++team) result.Teams[team] = roster.Teams[team];
        result.Auto = roster.Auto;
        return result;
    }

    // return: The roster of the engine as the one of the bridge, a count capped at a byte.
    auto ToBridgeRoster(const Roster& roster) -> BridgeRoster
    {
        BridgeRoster result{};
        for (std::size_t team{ 0 }; team < result.Teams.size(); ++team) result.Teams[team] = static_cast<std::uint8_t>((std::min)(roster.Teams[team], std::size_t{ 255 }));
        result.Auto = static_cast<std::uint8_t>((std::min)(roster.Auto, std::size_t{ 255 }));
        return result;
    }

    // return: The text as ASCII that fits the reason field of the bridge: other characters become '?', and what does not fit is cut.
    auto ToReason(std::string_view text) -> std::string
    {
        std::string reason{};
        for (const char character : text.substr(0, k_BridgeSetupReasonSize))
        {
            reason += (character >= ' ' && character <= '~') ? character : '?';
        }
        return reason;
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

    auto BridgeEngineAdapter::RequestSetup(const BridgeSetupRequest& request) -> void
    {
        // The files and the blocks are read here, on the bridge thread: the game thread only receives the values. One request carries
        // every field, so it is one setup for the service: no merge.
        MatchSetup setup{};

        if (request.Preset)
        {
            auto pick{ m_PresetService.Resolve(*request.Preset) };
            if (!pick)
            {
                m_MatchSetupService.Reject(pick.error());
                return;
            }
            setup.Preset = std::move(*pick);
        }

        if (request.Map)
        {
            // No name asks for the map variant the match loaded with.
            std::vector<std::byte> block{};
            if (!request.Map->empty()) block = m_MatchConfigService.ReadMapVariant(*request.Map);

            auto mapVariant{ m_MapVariantService.Resolve(*request.Map, std::move(block)) };
            if (!mapVariant)
            {
                m_MatchSetupService.Reject(mapVariant.error());
                return;
            }
            setup.Map = std::move(*mapVariant);
        }

        if (request.Roster) setup.Bots = ToRoster(*request.Roster);

        m_MatchSetupService.Request(std::move(setup), MatchSetupOrigin::Bridge);
    }

    auto BridgeEngineAdapter::CancelSetupRequest() -> void
    {
        m_MatchSetupService.Cancel(MatchSetupOrigin::Bridge);
    }

    auto BridgeEngineAdapter::GetSetupResult() const -> BridgeSetupInfo
    {
        const auto result{ m_MatchSetupService.GetResult() };
        return BridgeSetupInfo{ .Result = ToSetupResult(result.Outcome), .Round = result.Round, .Reason = ToReason(result.Reason) };
    }

    auto BridgeEngineAdapter::GetActiveSetup() const -> BridgeActiveSetup
    {
        auto active{ m_MatchSetupService.GetActive() };
        BridgeActiveSetup setup{ .Preset = std::move(active.Preset), .Map = std::move(active.Map) };
        if (active.Bots) setup.Roster = ToBridgeRoster(*active.Bots);
        return setup;
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