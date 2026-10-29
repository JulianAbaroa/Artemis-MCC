export module Perception.Match.System;

import Blam.Runtime.Player.State;
import Blam.Runtime.Game.State;
import Perception.Match.State;
import std;

export namespace Perception::Match::System
{
    // Builds the state of the match from the game globals and the player table (D103).
    class MatchService
    {
    private:
        using PlayerTableStore = Blam::Runtime::Player::State::PlayerTableStore;
        using GameStore = Blam::Runtime::Game::State::GameStore;
        using MatchStore = Perception::Match::State::MatchStore;

    public:
        MatchService(PlayerTableStore& playerStore, GameStore& gameStore, MatchStore& matchStore) :
            m_PlayerStore(playerStore), m_GameStore(gameStore), m_MatchStore(matchStore) {}
        ~MatchService() = default;

        // Publishes the clock, the phase and the score of each player of the tick.
        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        PlayerTableStore& m_PlayerStore;
        GameStore& m_GameStore;
        MatchStore& m_MatchStore;
    };
}