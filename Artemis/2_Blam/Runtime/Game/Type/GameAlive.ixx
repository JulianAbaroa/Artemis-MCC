export module Blam.Runtime.Game.Type:Alive;

import Common.Engine.Type;
import Blam.Layout.Game.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_MaxTeams;
    using Blam::Layout::Game::Type::Offset::k_EventDataSize;
    using Blam::Layout::Game::Type::Offset::k_PowerupCount;
}

export namespace Blam::Runtime::Game::Type::Alive
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

    // Score of one team: in the current round and added up over the match.
    struct AliveTeamScore
    {
        std::int16_t Round{};
        std::int16_t Total{};
    };

    // One element of the megalo objects array that holds an object: the state the gametype script keeps for it (D250).
    struct AliveMegaloObject
    {
        std::uint32_t ObjectHandle{};
        // Raw engine value. For a flag 0 home, 1 carried, 2 dropped (A14).
        std::uint32_t State{};
        // Raw engine value: high byte 5 + allies in the zone while it runs (0 paused), low 24 bits time in 1/1200 s (A14).
        std::uint32_t ResetTimer{};
    };

    // Megalo elements of one tick that hold an object, in element order.
    struct AliveMegaloObjects : std::vector<AliveMegaloObject>
    {
        using vector::vector;
    };

    // Game globals of one tick: clock, phase, scores and the new entries of the event ring.
    struct AliveGame
    {
        // Ticks since the start of the match (time globals +0xC).
        std::uint32_t EngineTick{};
        // Whether the game engine globals were captured in this tick.
        bool HasEngineGlobals{ false };
        std::array<std::int16_t, k_MaxPlayers> PlayerScores{};

        // Round of the match, from 0. Zero when the game engine globals were not captured.
        std::int16_t RoundIndex{};
        // Raw state of the round (1 in play, 2 ended, 3 between rounds).
        std::uint8_t RoundState{};
        // Team that plays each side, 0xFF for a side not in use. Indexed by the Team of a placement.
        std::array<std::uint8_t, k_MaxTeams> TeamMapping{};
        // Score of each team, indexed by team.
        std::array<AliveTeamScore, k_MaxTeams> TeamScores{};
        // Ticks the round has been played. With a time limit it is the limit minus the ticks left (0 when the round ran out), without
        // one the ticks of the stopwatch. 0 when the game engine globals were not captured.
        std::uint32_t RoundClockTicks{};
        // Ticks left in the round: the limit counts down to 0. -1 when the round has no time limit.
        std::int32_t RoundTicksRemaining{ -1 };
        // Time limit of a round in ticks, 0 for none or when the game variant is not available.
        std::uint32_t RoundTimeLimitTicks{};
        // Limit of rounds of the game variant as stored, 0 when the game variant is not available.
        std::uint8_t RoundLimit{};
        // Score to win of the game variant as stored, 0 for none or when the variant is not available or not Megalo.
        std::uint16_t ScoreToWin{};
        // Respawn, round and powerup settings of the game variant (D262). All 0 when the game variant is not available.
        // Seconds of the variant in ticks. -1 stays -1 (sudden death indefinite).
        std::uint32_t RespawnTimeTicks{};
        std::uint8_t LivesPerRound{};
        std::uint8_t TeamLivesPerRound{};
        std::uint8_t RoundsToWin{};
        std::int32_t SuddenDeathTicks{};
        std::uint32_t GracePeriodTicks{};
        // Duration of the red, blue and yellow powerups in ticks.
        std::array<std::uint32_t, k_PowerupCount> PowerupDurationTicks{};
        // Megalo state per object. Empty when the region was not captured.
        AliveMegaloObjects MegaloObjects{};

        // Entries whose sequence is above the last one read the tick before.
        AliveEvents Events{};
        // Last sequence written by the engine.
        std::uint32_t LastSequence{};
        // Entries overwritten before they were read, since the engine started.
        std::uint64_t DroppedEvents{};
    };
}