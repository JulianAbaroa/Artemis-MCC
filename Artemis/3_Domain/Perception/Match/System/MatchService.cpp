module Perception.Match.System;

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.Runtime.Player.Type;
import Blam.Runtime.Game.Type;
import Perception.Events.Type;
import Perception.Match.Type;

namespace
{
    using Common::Semantic::Type::MatchPhase;
    using Common::Semantic::Type::RoundState;
    using Perception::Match::Type::MatchStatus;
    using Perception::Match::Type::MatchScore;
    using Perception::Match::Type::MatchTeamScore;

    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_MaxTeams;

    // return: The phase of the match: InGame from the start until the GameOver event of the engine, Ended from the tick it was read, Unknown without
    // the game engine globals.
    // note: The raw phase of the engine is not used: it is also Ended between the end of a round and the next one and for a tick when the local
    // player dies (A14).
    auto ToPhase(bool hasEngineGlobals, std::optional<std::uint32_t> gameOverTick, std::uint32_t gameTick) -> MatchPhase
    {
        if (!hasEngineGlobals) return MatchPhase::Unknown;
        return gameOverTick && gameTick >= *gameOverTick ? MatchPhase::Ended : MatchPhase::InGame;
    }

    // return: The state of the round of the raw value of the engine.
    auto ToRoundState(std::uint8_t state) -> RoundState
    {
        switch (state)
        {
        case 1: return RoundState::InPlay;
        case 2: return RoundState::Ended;
        case 3: return RoundState::Between;
        default: return RoundState::Unknown;
        }
    }
}

namespace Perception::Match::System
{
    auto MatchService::Update() -> void
    {
        const auto game{ m_GameStore.Acquire() };
        if (!game) return;

        MatchStatus match{ .GameTick = game->EngineTick, .Phase = ToPhase(game->HasEngineGlobals, m_EventsMemoryStore.Get().GameOverTick, game->EngineTick), .RoundIndex = game->RoundIndex, .RoundState = ToRoundState(game->RoundState),
            .RoundClockTicks = game->RoundClockTicks, .RoundTicksRemaining = game->RoundTicksRemaining,
            .RoundTimeLimitTicks = game->RoundTimeLimitTicks, .RoundLimit = game->RoundLimit, .ScoreToWin = game->ScoreToWin,
            .RespawnTimeTicks = game->RespawnTimeTicks, .LivesPerRound = game->LivesPerRound, .TeamLivesPerRound = game->TeamLivesPerRound,
            .RoundsToWin = game->RoundsToWin, .SuddenDeathTicks = game->SuddenDeathTicks, .GracePeriodTicks = game->GracePeriodTicks,
            .PowerupDurationTicks = game->PowerupDurationTicks };

        for (std::size_t team{ 0 }; team < k_MaxTeams; ++team)
        {
            match.TeamScores[team] = MatchTeamScore{ .Round = game->TeamScores[team].Round, .Total = game->TeamScores[team].Total };
        }

        if (const auto players{ m_PlayerStore.Acquire() })
        {
            for (const auto& entry : *players)
            {
                const std::size_t index{ entry.first & k_HandleIndexMask };
                if (index >= k_MaxPlayers) continue;

                match.Scores.push_back(MatchScore{ .PlayerHandle = entry.first, .Score = game->PlayerScores[index] });
            }

            std::ranges::sort(match.Scores, {}, [](const MatchScore& score) { return score.PlayerHandle & k_HandleIndexMask; });
        }

        m_MatchStore.Publish(std::move(match));
    }

    auto MatchService::Cleanup() -> void
    {
        m_MatchStore.Cleanup();
    }
}