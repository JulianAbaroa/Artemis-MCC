module Tables.Game.System;

import Common.Engine.Type;
import Datum.Object.Type;

namespace
{
    using Tables::Game::Type::Alive::AliveEvent;

    using Common::Engine::Type::k_MaxPlayers;
    using Datum::Object::Type::Constant::k_ThreadContextTimeGlobalsOffset;
    using Datum::Object::Type::Constant::k_TimeGlobalsEngineTickOffset;
    using Datum::Object::Type::Constant::k_ThreadContextGameEngineOffset;
    using Datum::Object::Type::Constant::k_ThreadContextEventGlobalsOffset;
    using Datum::Object::Type::Constant::k_EventGlobalsSequenceOffset;
    using Tables::Game::Type::Offset::k_GameEnginePhase;
    using Tables::Game::Type::Offset::k_GameEnginePlayerScores;
    using Tables::Game::Type::Offset::k_PlayerScoreStride;
    using Tables::Game::Type::Offset::k_EventRing;
    using Tables::Game::Type::Offset::k_EventRingSlots;
    using Tables::Game::Type::Offset::k_EventEntrySize;
    using Tables::Game::Type::Offset::k_EntryKey;
    using Tables::Game::Type::Offset::k_EntryData;
    using Tables::Game::Type::Offset::k_EntrySequence;
    using Tables::Game::Type::Offset::k_EntryGameTick;
    using Tables::Game::Type::Offset::k_EventDataSize;
    using Tables::Game::Type::Offset::k_DataCausePlayer;
    using Tables::Game::Type::Offset::k_DataCauseObject;
    using Tables::Game::Type::Offset::k_DataEffectPlayer;
    using Tables::Game::Type::Offset::k_DataEffectObject;
    using Tables::Game::Type::Offset::k_DataDamageReportingType;
    using Tables::Game::Type::Offset::k_DataModifier;
    using Tables::Game::Type::Offset::k_DataExtra;

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
    auto GameService::Update() -> void
    {
        const std::uintptr_t threadContext{ m_DatumStore.GetThreadContext() };
        if (threadContext == 0) return;

        auto& reader = m_MemoryReaderService;
        AliveGame game{};

        const auto timeGlobals{ reader.Read<std::uintptr_t>(threadContext, k_ThreadContextTimeGlobalsOffset) };
        if (timeGlobals != 0) game.EngineTick = reader.Read<std::uint32_t>(timeGlobals, k_TimeGlobalsEngineTickOffset);

        const auto gameEngine{ reader.Read<std::uintptr_t>(threadContext, k_ThreadContextGameEngineOffset) };
        if (gameEngine != 0)
        {
            game.Phase = reader.Read<std::uint32_t>(gameEngine, k_GameEnginePhase);

            for (std::size_t index{ 0 }; index < k_MaxPlayers; ++index)
            {
                game.PlayerScores[index] = reader.Read<std::int16_t>(gameEngine, k_GameEnginePlayerScores + index * k_PlayerScoreStride);
            }
        }

        const auto eventGlobals{ reader.Read<std::uintptr_t>(threadContext, k_ThreadContextEventGlobalsOffset) };
        if (eventGlobals != 0) this->ReadEvents(eventGlobals, game);

        // The phase values are measured on few game modes (A5): every change is logged to confirm them on the others.
        const auto previous{ m_GameStore.Acquire() };
        if (!previous || previous->Phase != game.Phase)
        {
            m_LogsService.Message("[GameService] INFO: Game engine phase {} at engine tick {}.", game.Phase, game.EngineTick);
        }

        game.DroppedEvents = m_GameStore.GetDroppedEvents();
        m_GameStore.Publish(std::move(game));
    }

    auto GameService::Cleanup() -> void
    {
        m_GameStore.Cleanup();
    }

    auto GameService::ReadEvents(std::uintptr_t eventGlobals, AliveGame& game) -> void
    {
        auto& reader = m_MemoryReaderService;

        const auto last{ reader.Read<std::uint32_t>(eventGlobals, k_EventGlobalsSequenceOffset) };
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
                m_LogsService.Message("[GameService] WARNING: {} events of the ring were overwritten before they were read.", dropped);
            }
        }

        game.Events.reserve(last - first + 1);
        std::size_t mismatches{ 0 };

        for (std::uint32_t sequence{ first }; sequence <= last; ++sequence)
        {
            const std::uintptr_t entry{ eventGlobals + k_EventRing + (sequence % k_EventRingSlots) * k_EventEntrySize };
            if (reader.Read<std::uint32_t>(entry, k_EntrySequence) != sequence)
            {
                ++mismatches;
                continue;
            }

            AliveEvent event{};
            event.Sequence = sequence;
            event.GameTick = reader.Read<std::uint32_t>(entry, k_EntryGameTick);
            event.Key = reader.Read<std::uint32_t>(entry, k_EntryKey);
            event.Data = reader.ReadArray<std::uint8_t, k_EventDataSize>(entry, k_EntryData);

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
            m_LogsService.Message("[GameService] WARNING: {} entries of the event ring held another sequence (layout changed?).", mismatches);
        }

        m_GameStore.SetEventCursor(last);
    }
}