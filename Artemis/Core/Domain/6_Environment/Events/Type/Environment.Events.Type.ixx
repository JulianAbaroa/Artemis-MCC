export module Environment.Events.Type;

import Common.Semantic.Type;
import Tables.Game.Type;
import std;

namespace
{
    using Common::Semantic::Type::Event;

    using Tables::Game::Type::Offset::k_EventDataSize;
}

export namespace Environment::Events::Type
{
    // Events the log keeps: enough for every reader that samples less often than each tick (the bridge, the bench).
    constexpr std::size_t k_EventLogCapacity{ 256 };

    // One event of the engine (D104).
    struct GameEvent
    {
        // Sequence of the engine: from 1 in each match, without gaps. Readers keep the last one they handled.
        std::uint32_t Sequence{};
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

    // Last events of the match, oldest first.
    struct EventLog
    {
        std::vector<GameEvent> Rows{};
        // Sequence of the newest event, 0 before the first one.
        std::uint32_t LastSequence{};
    };
}