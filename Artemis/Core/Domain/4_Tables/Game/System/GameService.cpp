module Tables.Game.System;

import Common.Engine.Type;
import Datum.Object.Type;
import Layout.Game.Type;
import Layout.Snapshot.Type;

namespace
{
    using Tables::Game::Type::Alive::AliveEvent;

    using Layout::Snapshot::Type::RegionId;

    using Common::Engine::Type::k_MaxPlayers;
    using Datum::Object::Type::Constant::k_TimeGlobalsEngineTickOffset;
    using Datum::Object::Type::Constant::k_EventGlobalsSequenceOffset;
    using Layout::Game::Type::Offset::k_GameEnginePhase;
    using Layout::Game::Type::Offset::k_GameEnginePlayerScores;
    using Layout::Game::Type::Offset::k_PlayerScoreStride;
    using Layout::Game::Type::Offset::k_EventRing;
    using Layout::Game::Type::Offset::k_EventRingSlots;
    using Layout::Game::Type::Offset::k_EventEntrySize;
    using Layout::Game::Type::Offset::k_EntryKey;
    using Layout::Game::Type::Offset::k_EntryData;
    using Layout::Game::Type::Offset::k_EntrySequence;
    using Layout::Game::Type::Offset::k_EntryGameTick;
    using Layout::Game::Type::Offset::k_EventDataSize;
    using Layout::Game::Type::Offset::k_DataCausePlayer;
    using Layout::Game::Type::Offset::k_DataCauseObject;
    using Layout::Game::Type::Offset::k_DataEffectPlayer;
    using Layout::Game::Type::Offset::k_DataEffectObject;
    using Layout::Game::Type::Offset::k_DataDamageReportingType;
    using Layout::Game::Type::Offset::k_DataModifier;
    using Layout::Game::Type::Offset::k_DataExtra;

    // return: The field of type T at the offset of the event data.
    template <typename T>
    auto FieldOf(const std::array<std::uint8_t, k_EventDataSize>& data, std::uintptr_t offset) -> T
    {
        T value{};
        std::memcpy(&value, data.data() + offset, sizeof(T));
        return value;
    }
}

namespace Tables::Game::System
{
    auto GameService::Update(const Snapshot& snapshot) -> void
    {
        if (!snapshot.HasThreadContext) return;

        AliveGame game{};

        if (snapshot.IsCaptured(RegionId::TimeGlobals)) game.EngineTick = snapshot.Read<std::uint32_t>(RegionId::TimeGlobals, k_TimeGlobalsEngineTickOffset);

        if (snapshot.IsCaptured(RegionId::GameEngineGlobals))
        {
            game.Phase = snapshot.Read<std::uint32_t>(RegionId::GameEngineGlobals, k_GameEnginePhase);

            for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
            {
                game.PlayerScores[index] = snapshot.Read<std::int16_t>(RegionId::GameEngineGlobals, k_GameEnginePlayerScores + index * k_PlayerScoreStride);
            }
        }

        if (snapshot.IsCaptured(RegionId::EventGlobals)) this->ReadEvents(snapshot, game);

        // The phase values are measured on few game modes (A5): every change is logged to confirm them on the others.
        const auto previous{ m_GameStore.Acquire() };
        if (!previous || previous->Phase != game.Phase)
        {
            m_Log.Message("[GameService] INFO: Game engine phase {} at engine tick {}.", game.Phase, game.EngineTick);
        }

        game.DroppedEvents = m_GameStore.GetDroppedEvents();
        m_GameStore.Publish(std::move(game));
    }

    auto GameService::Cleanup() -> void
    {
        m_GameStore.Cleanup();
    }

    auto GameService::ReadEvents(const Snapshot& snapshot, AliveGame& game) -> void
    {
        const auto last{ snapshot.Read<std::uint32_t>(RegionId::EventGlobals, k_EventGlobalsSequenceOffset) };
        std::uint32_t cursor{ m_GameStore.GetEventCursor() };

        // A new engine starts the sequence again.
        if (last < cursor) cursor = 0;

        game.LastSequence = last;
        if (last == cursor) return;

        std::uint32_t first{ cursor + 1 };
        if (last - cursor > k_EventRingSlots)
        {
            first = last - k_EventRingSlots + 1;

            // On the first read of an engine the older entries are from before Artemis saw the match, not a loss.
            if (cursor != 0)
            {
                const std::uint64_t dropped{ first - cursor - 1 };
                m_GameStore.AddDroppedEvents(dropped);
                m_Log.Message("[GameService] WARNING: {} events of the ring were overwritten before they were read.", dropped);
            }
        }

        game.Events.reserve(last - first + 1);
        std::size_t mismatches{ 0 };

        for (std::uint32_t sequence{ first }; sequence <= last; ++sequence)
        {
            const std::size_t entry{ k_EventRing + (sequence % k_EventRingSlots) * k_EventEntrySize };
            if (snapshot.Read<std::uint32_t>(RegionId::EventGlobals, entry + k_EntrySequence) != sequence)
            {
                ++mismatches;
                continue;
            }

            AliveEvent event{};
            event.Sequence = sequence;
            event.GameTick = snapshot.Read<std::uint32_t>(RegionId::EventGlobals, entry + k_EntryGameTick);
            event.Key = snapshot.Read<std::uint32_t>(RegionId::EventGlobals, entry + k_EntryKey);
            event.Data = snapshot.ReadArray<std::uint8_t, k_EventDataSize>(RegionId::EventGlobals, entry + k_EntryData);

            event.CausePlayer = FieldOf<std::uint32_t>(event.Data, k_DataCausePlayer);
            event.CauseObject = FieldOf<std::uint32_t>(event.Data, k_DataCauseObject);
            event.EffectPlayer = FieldOf<std::uint32_t>(event.Data, k_DataEffectPlayer);
            event.EffectObject = FieldOf<std::uint32_t>(event.Data, k_DataEffectObject);
            event.DamageReportingType = FieldOf<std::uint8_t>(event.Data, k_DataDamageReportingType);
            event.Modifier = FieldOf<std::uint8_t>(event.Data, k_DataModifier);
            event.Extra = FieldOf<std::uint32_t>(event.Data, k_DataExtra);

            game.Events.push_back(event);
        }

        if (mismatches > 0)
        {
            m_Log.Message("[GameService] WARNING: {} entries of the event ring held another sequence (layout changed?).", mismatches);
        }

        m_GameStore.SetEventCursor(last);
    }
}