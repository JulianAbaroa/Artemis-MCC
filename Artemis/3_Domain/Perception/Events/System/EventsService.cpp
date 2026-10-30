module Perception.Events.System;

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.Layout.Game.Type;

namespace
{
    using Perception::Events::Type::EventLog;
    using Perception::Events::Type::EventsMemory;
    using Perception::Events::Type::PendingCapture;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ObjectiveDetails;

    using Common::Semantic::Type::Event;
    using Common::Semantic::Type::FlagState;
    using Common::Semantic::Type::EventKey::KindOf;
    using Common::Engine::Type::k_MaxTeams;
    using Common::Engine::Type::k_NoHandle;
    using Perception::Events::Type::k_EventLogCapacity;
    using Blam::Layout::Game::Type::Offset::k_ModifierHeadshot;
    using Blam::Layout::Game::Type::Offset::k_ModifierStuck;

    // return: Whether the kind is one of the flag events, which Artemis generates itself.
    auto IsFlagKind(Event kind) -> bool
    {
        switch (kind)
        {
        case Event::FlagTaken:
        case Event::FlagDropped:
        case Event::FlagReturned:
        case Event::FlagReset:
        case Event::FlagCaptured: return true;
        default: return false;
        }
    }

    // return: A flag event of Artemis: no key and no sequence of the engine.
    // param flag: The flag the event is about. Its OwnerTeam is the modifier.
    auto MakeFlagEvent(Event kind, std::uint32_t gameTick, const ObjectiveDetail& flag, std::uint32_t carrierPlayer, std::uint32_t carrierBiped,
        std::uint32_t effectObject) -> Perception::Events::Type::GameEvent
    {
        return Perception::Events::Type::GameEvent{
            .GameTick = gameTick,
            .Kind = kind,
            .CausePlayer = carrierPlayer,
            .CauseObject = carrierBiped,
            .EffectPlayer = k_NoHandle,
            .EffectObject = effectObject,
            .Modifier = static_cast<std::uint8_t>(flag.OwnerTeam),
        };
    }
}

namespace Perception::Events::System
{
    auto EventsService::Update() -> void
    {
        const auto game{ m_GameStore.Acquire() };
        if (!game) return;

        EventsMemory& memory{ m_MemoryStore.Get() };

        std::vector<GameEvent> fresh{};
        fresh.reserve(game->Events.size());
        for (const AliveEvent& entry : game->Events)
        {
            const GameEvent event{ this->MakeEvent(entry) };
            if (!IsFlagKind(event.Kind)) fresh.push_back(event);
        }

        // note: Read before the flag events are appended, because the engine events of a match start again at 1 only with a new engine.
        const bool isSameMatch{ game->Events.empty() || game->Events.front().Sequence > memory.LastEngineSequence };
        if (!game->Events.empty()) memory.LastEngineSequence = game->Events.back().Sequence;

        this->AppendFlagEvents(*game, fresh);
        if (fresh.empty()) return;

        const auto previous{ m_EventsStore.Acquire() };

        EventLog log{ previous && isSameMatch ? *previous : EventLog{} };
        for (GameEvent& event : fresh)
        {
            event.Sequence = ++log.LastSequence;
            log.Rows.push_back(event);
        }

        if (log.Rows.size() > k_EventLogCapacity)
        {
            log.Rows.erase(log.Rows.begin(), log.Rows.end() - static_cast<std::ptrdiff_t>(k_EventLogCapacity));
        }

        m_EventsStore.Publish(std::move(log));
    }

    auto EventsService::Cleanup() -> void
    {
        m_MemoryStore.Cleanup();
        m_EventsStore.Cleanup();
    }

    auto EventsService::AppendFlagEvents(const AliveGame& game, std::vector<GameEvent>& events) -> void
    {
        const std::uint32_t gameTick{ game.EngineTick };
        const std::int16_t roundIndex{ game.RoundIndex };
        EventsMemory& memory{ m_MemoryStore.Get() };

        std::uint32_t score{ 0 };
        for (const auto& teamScore : game.TeamScores)
        {
            score += teamScore.Round > 0 ? static_cast<std::uint32_t>(teamScore.Round) : 0u;
            score += teamScore.Total > 0 ? static_cast<std::uint32_t>(teamScore.Total) : 0u;
        }
        const bool isScoreUp{ score > memory.LastScore };
        const std::uint32_t scoreBefore{ memory.LastScore };
        memory.LastScore = score;
        const auto objectives{ m_ObjectiveDetailsStore.Acquire() };

        // The flag of each base team in this tick.
        std::array<const ObjectiveDetail*, k_MaxTeams> current{};
        if (objectives)
        {
            for (const auto& [handle, flag] : objectives->Flags)
            {
                const std::size_t team{ static_cast<std::size_t>(flag.BaseTeam) };
                if (team < k_MaxTeams) current[team] = &flag;
            }
        }

        const bool isNewRound{ !memory.HasRound || memory.RoundIndex != roundIndex };
        memory.RoundIndex = roundIndex;
        memory.HasRound = true;

        // A pending capture that got its score is a capture, one that did not by the next tick is a reset.
        for (std::size_t team{ 0 }; team < k_MaxTeams; ++team)
        {
            auto& pending{ memory.Pending[team] };
            if (!pending) continue;

            if (isNewRound)
            {
                pending.reset();
                continue;
            }

            if (score > pending->ScoreBefore)
            {
                events.push_back(MakeFlagEvent(Event::FlagCaptured, pending->Tick, pending->Flag, pending->Flag.CarrierPlayer, pending->Flag.CarrierBiped, pending->Flag.Handle));
                pending.reset();
            }
            else if (gameTick > pending->Tick)
            {
                events.push_back(MakeFlagEvent(Event::FlagReset, pending->Tick, pending->Flag, k_NoHandle, k_NoHandle, pending->Flag.Handle));
                pending.reset();
            }
        }

        // Called for a carried flag that went home: now if a score went up in this tick, else when the next tick says.
        const auto captured{ [&](std::size_t team, const ObjectiveDetail& flag) {
            if (isScoreUp)
            {
                events.push_back(MakeFlagEvent(Event::FlagCaptured, gameTick, flag, flag.CarrierPlayer, flag.CarrierBiped, flag.Handle));
            }
            else
            {
                memory.Pending[team] = PendingCapture{ .Flag = flag, .Tick = gameTick, .ScoreBefore = scoreBefore };
            }
        } };

        for (std::size_t team{ 0 }; team < k_MaxTeams; ++team)
        {
            const ObjectiveDetail* now{ current[team] };
            auto& before{ memory.Flags[team] };

            // The first tick after a load and a change of round only record the flags.
            if (isNewRound)
            {
                before = now != nullptr ? std::optional<ObjectiveDetail>{ *now } : std::nullopt;
                continue;
            }

            if (!before)
            {
                if (now != nullptr) before = *now;
                continue;
            }

            const ObjectiveDetail previous{ *before };

            if (now == nullptr)
            {
                // Carried and gone: captured. Remember it at home so the flag that comes back does not capture again.
                if (previous.State == FlagState::Carried)
                {
                    captured(team, previous);
                    before->State = FlagState::Home;
                    before->CarrierPlayer = k_NoHandle;
                    before->CarrierBiped = k_NoHandle;
                }

                continue;
            }

            const bool isRecreated{ now->Handle != previous.Handle };
            if (isRecreated)
            {
                if (previous.State == FlagState::Carried)
                {
                    captured(team, previous);
                }
                else if (previous.State == FlagState::Dropped)
                {
                    const Event kind{ previous.AlliesInZone > 0 ? Event::FlagReturned : Event::FlagReset };
                    events.push_back(MakeFlagEvent(kind, gameTick, previous, k_NoHandle, k_NoHandle, previous.Handle));
                }

                if (now->State == FlagState::Carried)
                {
                    events.push_back(MakeFlagEvent(Event::FlagTaken, gameTick, *now, now->CarrierPlayer, now->CarrierBiped, now->Handle));
                }
            }
            else if (previous.State != FlagState::Carried && now->State == FlagState::Carried)
            {
                events.push_back(MakeFlagEvent(Event::FlagTaken, gameTick, *now, now->CarrierPlayer, now->CarrierBiped, now->Handle));
            }
            else if (previous.State == FlagState::Carried && now->State == FlagState::Dropped)
            {
                events.push_back(MakeFlagEvent(Event::FlagDropped, gameTick, *now, previous.CarrierPlayer, previous.CarrierBiped, now->Handle));
            }

            // note: Only a Carried flag needs its carrier later (the drop and the capture read it from the tick before).
            before = *now;
        }
    }

    auto EventsService::MakeEvent(const AliveEvent& entry) -> GameEvent
    {
        return GameEvent{
            .EngineSequence = entry.Sequence,
            .GameTick = entry.GameTick,
            .Kind = KindOf(entry.Key),
            .Key = entry.Key,
            .CausePlayer = entry.CausePlayer,
            .CauseObject = entry.CauseObject,
            .EffectPlayer = entry.EffectPlayer,
            .EffectObject = entry.EffectObject,
            .DamageReportingType = entry.DamageReportingType,
            .IsHeadshot = entry.Modifier == k_ModifierHeadshot,
            .IsStuck = entry.Modifier == k_ModifierStuck,
            .Modifier = entry.Modifier,
            .Extra = entry.Extra,
            .Data = entry.Data,
        };
    }
}