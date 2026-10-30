module Perception.Match.System;

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.Layout.Game.Type;
import Blam.Runtime.Player.Type;
import Blam.Runtime.Game.Type;
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
    using Blam::Layout::Game::Type::Offset::k_PhaseMask;
    using Blam::Layout::Game::Type::Offset::k_PhaseInGame;
    using Blam::Layout::Game::Type::Offset::k_PhaseEnded;

    // return: The phase of the raw value of the engine.
    auto ToPhase(std::uint32_t phase) -> MatchPhase
    {
        if ((phase & k_PhaseMask) == k_PhaseInGame) return MatchPhase::InGame;
        if ((phase & k_PhaseMask) == k_PhaseEnded) return MatchPhase::Ended;
        return MatchPhase::Unknown;
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

        MatchStatus match{ .GameTick = game->EngineTick, .Phase = ToPhase(game->Phase), .RoundIndex = game->RoundIndex, .RoundState = ToRoundState(game->RoundState),
            .RoundClockTicks = game->RoundClockTicks, .RoundTicksRemaining = game->RoundTicksRemaining,
            .RoundTimeLimitTicks = game->RoundTimeLimitTicks, .RoundLimit = game->RoundLimit };

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