module Environment.Events.System;

import Common.Semantic.Type;
import Layout.Game.Type;

namespace
{
    using Environment::Events::Type::EventLog;

    using Common::Semantic::Type::EventKey::KindOf;
    using Environment::Events::Type::k_EventLogCapacity;
    using Layout::Game::Type::Offset::k_ModifierHeadshot;
    using Layout::Game::Type::Offset::k_ModifierStuck;
}

namespace Environment::Events::System
{
    auto EventsService::Update() -> void
    {
        const auto game{ m_GameStore.Acquire() };
        if (!game || game->Events.empty()) return;

        const auto previous{ m_EventsStore.Acquire() };
        const bool isSameMatch{ previous && game->Events.front().Sequence > previous->LastSequence };

        EventLog log{ isSameMatch ? *previous : EventLog{} };
        for (const AliveEvent& entry : game->Events)
        {
            log.Rows.push_back(this->MakeEvent(entry));
        }

        if (log.Rows.size() > k_EventLogCapacity)
        {
            log.Rows.erase(log.Rows.begin(), log.Rows.end() - static_cast<std::ptrdiff_t>(k_EventLogCapacity));
        }

        log.LastSequence = log.Rows.back().Sequence;
        m_EventsStore.Publish(std::move(log));
    }

    auto EventsService::Cleanup() -> void
    {
        m_EventsStore.Cleanup();
    }

    auto EventsService::MakeEvent(const AliveEvent& entry) -> GameEvent
    {
        return GameEvent{
            .Sequence = entry.Sequence,
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