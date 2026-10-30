module Blam.Runtime.Game.System;

import Common.Engine.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Game.Type;
import Blam.Layout.Snapshot.Type;

namespace
{
    using Blam::Runtime::Game::Type::Alive::AliveEvent;

    // Ticks in a minute of a round time limit (60 s of 60 ticks).
    constexpr std::uint32_t k_TicksPerMinute{ 3600 };

    using Blam::Layout::Snapshot::Type::RegionId;

    using Blam::Runtime::Game::Type::Alive::AliveMegaloObject;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_MaxTeams;
    using Common::Engine::Type::k_NoHandle;
    using Blam::Layout::Object::Type::Constant::k_TimeGlobalsEngineTickOffset;
    using Blam::Layout::Object::Type::Constant::k_EventGlobalsSequenceOffset;
    using Blam::Layout::Object::Type::Constant::k_MaxMegaloObjects;
    using Blam::Layout::Object::Type::Constant::k_MegaloObjectSize;
    using Blam::Layout::Game::Type::Offset::k_GameEnginePhase;
    using Blam::Layout::Game::Type::Offset::k_GameEnginePlayerScores;
    using Blam::Layout::Game::Type::Offset::k_PlayerScoreStride;
    using Blam::Layout::Game::Type::Offset::k_GameEngineRoundIndex;
    using Blam::Layout::Game::Type::Offset::k_GameEngineRoundState;
    using Blam::Layout::Game::Type::Offset::k_GameEngineTeamMapping;
    using Blam::Layout::Game::Type::Offset::k_GameEngineTeamScoreRound;
    using Blam::Layout::Game::Type::Offset::k_GameEngineTeamScoreTotal;
    using Blam::Layout::Game::Type::Offset::k_TeamScoreStride;
    using Blam::Layout::Game::Type::Offset::k_GameEngineRoundTimer;
    using Blam::Layout::Game::Type::Offset::k_GameGlobalsGameVariant;
    using Blam::Layout::Game::Type::Offset::k_VariantModeFirst;
    using Blam::Layout::Game::Type::Offset::k_VariantModeLast;
    using Blam::Layout::Game::Type::Offset::k_VariantRoundLimit;
    using Blam::Layout::Game::Type::Offset::k_VariantRoundTimeLimit;
    using Blam::Layout::Game::Type::Offset::TimerTicks;
    using Blam::Layout::Object::Type::Constant::k_GameModeTypeOffset;
    using Blam::Layout::Game::Type::Offset::k_MegaloObjectHandle;
    using Blam::Layout::Game::Type::Offset::k_MegaloObjectState;
    using Blam::Layout::Game::Type::Offset::k_MegaloObjectResetTimer;
    using Blam::Layout::Game::Type::Offset::k_EventRing;
    using Blam::Layout::Game::Type::Offset::k_EventRingSlots;
    using Blam::Layout::Game::Type::Offset::k_EventEntrySize;
    using Blam::Layout::Game::Type::Offset::k_EntryKey;
    using Blam::Layout::Game::Type::Offset::k_EntryData;
    using Blam::Layout::Game::Type::Offset::k_EntrySequence;
    using Blam::Layout::Game::Type::Offset::k_EntryGameTick;
    using Blam::Layout::Game::Type::Offset::k_EventDataSize;
    using Blam::Layout::Game::Type::Offset::k_DataCausePlayer;
    using Blam::Layout::Game::Type::Offset::k_DataCauseObject;
    using Blam::Layout::Game::Type::Offset::k_DataEffectPlayer;
    using Blam::Layout::Game::Type::Offset::k_DataEffectObject;
    using Blam::Layout::Game::Type::Offset::k_DataDamageReportingType;
    using Blam::Layout::Game::Type::Offset::k_DataModifier;
    using Blam::Layout::Game::Type::Offset::k_DataExtra;

    // return: The field of type T at the offset of the event data.
    template <typename T>
    auto FieldOf(const std::array<std::uint8_t, k_EventDataSize>& data, std::uintptr_t offset) -> T
    {
        T value{};
        std::memcpy(&value, data.data() + offset, sizeof(T));
        return value;
    }
}

namespace Blam::Runtime::Game::System
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

            game.RoundIndex = snapshot.Read<std::int16_t>(RegionId::GameEngineGlobals, k_GameEngineRoundIndex);
            game.RoundState = snapshot.Read<std::uint8_t>(RegionId::GameEngineGlobals, k_GameEngineRoundState);
            game.TeamMapping = snapshot.ReadArray<std::uint8_t, k_MaxTeams>(RegionId::GameEngineGlobals, k_GameEngineTeamMapping);

            for (std::size_t team{ 0 }; team < k_MaxTeams; ++team)
            {
                game.TeamScores[team].Round = snapshot.Read<std::int16_t>(RegionId::GameEngineGlobals, k_GameEngineTeamScoreRound + team * k_TeamScoreStride);
                game.TeamScores[team].Total = snapshot.Read<std::int16_t>(RegionId::GameEngineGlobals, k_GameEngineTeamScoreTotal + team * k_TeamScoreStride);
            }
        }

        if (snapshot.IsCaptured(RegionId::MegaloObjects))
        {
            for (std::size_t element{ 0 }; element < k_MaxMegaloObjects; ++element)
            {
                const std::size_t start{ element * k_MegaloObjectSize };
                const auto handle{ snapshot.Read<std::uint32_t>(RegionId::MegaloObjects, start + k_MegaloObjectHandle) };
                if (handle == k_NoHandle) continue;

                game.MegaloObjects.push_back(AliveMegaloObject{
                    .ObjectHandle = handle,
                    .State = snapshot.Read<std::uint32_t>(RegionId::MegaloObjects, start + k_MegaloObjectState),
                    .ResetTimer = snapshot.Read<std::uint32_t>(RegionId::MegaloObjects, start + k_MegaloObjectResetTimer) });
            }
        }

        this->ReadRoundTime(snapshot, game);

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

    auto GameService::ReadRoundTime(const Snapshot& snapshot, AliveGame& game) -> void
    {
        // The game variant is only valid in the modes that have one.
        if (snapshot.IsCaptured(RegionId::GameGlobals))
        {
            const auto mode{ snapshot.Read<std::uint8_t>(RegionId::GameGlobals, k_GameModeTypeOffset) };
            if (mode >= k_VariantModeFirst && mode <= k_VariantModeLast)
            {
                game.RoundTimeLimitTicks = snapshot.Read<std::uint8_t>(RegionId::GameGlobals, k_GameGlobalsGameVariant + k_VariantRoundTimeLimit) * k_TicksPerMinute;
                game.RoundLimit = snapshot.Read<std::uint8_t>(RegionId::GameGlobals, k_GameGlobalsGameVariant + k_VariantRoundLimit);
            }
        }

        const bool hasLimit{ game.RoundTimeLimitTicks > 0 };
        game.RoundTicksRemaining = hasLimit ? 0 : -1;
        if (!snapshot.IsCaptured(RegionId::GameEngineGlobals)) return;

        const std::uint32_t ticks{ TimerTicks(snapshot.Read<std::uint32_t>(RegionId::GameEngineGlobals, k_GameEngineRoundTimer)) };
        if (!hasLimit)
        {
            game.RoundClockTicks = ticks;
            return;
        }

        game.RoundTicksRemaining = static_cast<std::int32_t>(ticks);
        game.RoundClockTicks = ticks < game.RoundTimeLimitTicks ? game.RoundTimeLimitTicks - ticks : 0;
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