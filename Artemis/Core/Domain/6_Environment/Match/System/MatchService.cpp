module Environment.Match.System;

import Common.Engine.Type;
import Common.Semantic.Type;
import Layout.Game.Type;
import Tables.Player.Type;
import Tables.Game.Type;
import Environment.Match.Type;

namespace
{
    using Common::Semantic::Type::MatchPhase;
    using Environment::Match::Type::MatchStatus;
    using Environment::Match::Type::MatchScore;

    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Engine::Type::k_MaxPlayers;
    using Layout::Game::Type::Offset::k_PhaseMask;
    using Layout::Game::Type::Offset::k_PhaseInGame;
    using Layout::Game::Type::Offset::k_PhaseEnded;

    // return: The phase of the raw value of the engine.
    auto ToPhase(std::uint32_t phase) -> MatchPhase
    {
        if ((phase & k_PhaseMask) == k_PhaseInGame) return MatchPhase::InGame;
        if ((phase & k_PhaseMask) == k_PhaseEnded) return MatchPhase::Ended;
        return MatchPhase::Unknown;
    }
}

namespace Environment::Match::System
{
    auto MatchService::Update() -> void
    {
        const auto game{ m_GameStore.Acquire() };
        if (!game) return;

        MatchStatus match{ .GameTick = game->EngineTick, .Phase = ToPhase(game->Phase) };

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