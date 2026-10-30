export module Perception.Events.Type;

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.Layout.Game.Type;
import Blam.Runtime.Game.Type;
import Perception.Details.Type;
import std;

namespace
{
    using Common::Semantic::Type::Event;

    using Blam::Layout::Game::Type::Offset::k_EventDataSize;
    using Perception::Details::Type::ObjectiveDetail;

    using Common::Engine::Type::k_MaxTeams;
}

export namespace Perception::Events::Type
{
    // Events the log keeps: enough for every reader that samples less often than each tick (the bridge, the bench).
    constexpr std::size_t k_EventLogCapacity{ 256 };

    // One event of the tick: an entry of the engine ring or a flag event Artemis generates (D104, D251).
    struct GameEvent
    {
        // Sequence of Artemis: from 1 in each match, without gaps, in the order the events are added (the engine events of the tick
        // first, then the flag events). Readers keep the last one they handled.
        std::uint32_t Sequence{};
        // Sequence of the engine ring, 0 in the events Artemis generates.
        std::uint32_t EngineSequence{};
        // Ticks since the start of the match.
        std::uint32_t GameTick{};
        Event Kind{};
        // Key of the engine (Semantic.xml eventkeys).
        std::uint32_t Key{};

        // Full handles, or the engine none value when the event has no such player or object.
        std::uint32_t CausePlayer{};
        std::uint32_t CauseObject{};
        std::uint32_t EffectPlayer{};
        std::uint32_t EffectObject{};

        std::uint8_t DamageReportingType{};
        bool IsHeadshot{ false };
        bool IsStuck{ false };

        // Raw until the measures of CTF name them (A14).
        std::uint8_t Modifier{};
        std::uint32_t Extra{};
        std::array<std::uint8_t, k_EventDataSize> Data{};
    };

    // A flag that left the carrier's hands by being recreated at home: a capture only if a score goes up in the same tick or the next.
    struct PendingCapture
    {
        // The flag as it was carried, with its last carrier.
        ObjectiveDetail Flag{};
        // Tick of the transition. The event keeps it even when it is generated a tick later.
        std::uint32_t Tick{};
        // Sum of the scores of the teams in the tick before the transition.
        std::uint32_t ScoreBefore{};
    };

    // What the events service remembers from the ticks before, to generate the flag events on the transitions (D251).
    struct EventsMemory
    {
        // Round of the last tick. No flag events are generated when it changes: the engine recreates the whole map.
        std::int16_t RoundIndex{};
        bool HasRound{ false };

        // Last engine sequence read, 0 before the first one.
        std::uint32_t LastEngineSequence{};

        // Sum of the round and match scores of every team in the last tick.
        std::uint32_t LastScore{};

        // Last flag seen of each base team (the handle changes when the engine recreates the flag).
        std::array<std::optional<ObjectiveDetail>, k_MaxTeams> Flags{};

        // Carried flags that went back home and wait for a score to be called a capture.
        std::array<std::optional<PendingCapture>, k_MaxTeams> Pending{};
    };

    // Last events of the match, oldest first.
    struct EventLog
    {
        std::vector<GameEvent> Rows{};
        // Sequence of the newest event, 0 before the first one.
        std::uint32_t LastSequence{};
    };
}