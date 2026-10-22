export module Tables.Game.Type:Alive;

import :Offset;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxPlayers;
    using Tables::Game::Type::Offset::k_EventDataSize;
}

export namespace Tables::Game::Type::Alive
{
    // One entry of the event ring of the engine, as read (D104).
    struct AliveEvent
    {
        std::uint32_t Sequence{};
        // Engine tick of the event (time globals +0xC).
        std::uint32_t GameTick{};
        std::uint32_t Key{};

        std::uint32_t CausePlayer{};
        std::uint32_t CauseObject{};
        std::uint32_t EffectPlayer{};
        std::uint32_t EffectObject{};

        std::uint8_t DamageReportingType{};
        std::uint8_t Modifier{};
        std::uint32_t Extra{};

        // Whole event data, for the fields that are not named yet (A14).
        std::array<std::uint8_t, k_EventDataSize> Data{};
    };

    // Entries of the event ring read in one tick, by sequence.
    struct AliveEvents : std::vector<AliveEvent>
    {
        using vector::vector;
    };

    // Game globals of one tick: clock, phase, scores and the new entries of the event ring.
    struct AliveGame
    {
        // Ticks since the start of the match (time globals +0xC).
        std::uint32_t EngineTick{};
        // Raw phase of the game engine (k_PhaseMask, k_PhaseInGame, k_PhaseEnded).
        std::uint32_t Phase{};
        std::array<std::int16_t, k_MaxPlayers> PlayerScores{};

        // Entries whose sequence is above the last one read the tick before.
        AliveEvents Events{};
        // Last sequence written by the engine.
        std::uint32_t LastSequence{};
        // Entries overwritten before they were read, since the engine started.
        std::uint64_t DroppedEvents{};
    };
}