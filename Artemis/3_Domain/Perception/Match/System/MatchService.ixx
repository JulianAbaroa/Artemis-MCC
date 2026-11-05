export module Perception.Match.System;

import Blam.Runtime.Player.State;
import Blam.Runtime.Game.State;
import Perception.Events.State;
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
        using EventsMemoryStore = Perception::Events::State::EventsMemoryStore;
        using MatchStore = Perception::Match::State::MatchStore;

    public:
        MatchService(PlayerTableStore& playerStore, GameStore& gameStore, EventsMemoryStore& eventsMemoryStore, MatchStore& matchStore) :
            m_PlayerStore(playerStore), m_GameStore(gameStore), m_EventsMemoryStore(eventsMemoryStore), m_MatchStore(matchStore) {}
        ~MatchService() = default;

        // Publishes the clock, the phase and the score of each player of the tick. The phase ends with the GameOver event that the Events service noted.
        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        PlayerTableStore& m_PlayerStore;
        GameStore& m_GameStore;
        EventsMemoryStore& m_EventsMemoryStore;
        MatchStore& m_MatchStore;
    };
}