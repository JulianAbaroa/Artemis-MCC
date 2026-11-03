export module Perception.Players.System;

import Common.Log.System;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Blam.Runtime.Player.Type;
import Blam.Runtime.Player.State;
import Blam.Runtime.Game.Type;
import Blam.Runtime.Game.State;
import Perception.Details.Type;
import Perception.Details.State;
import Perception.Entities.Type;
import Perception.Entities.State;
import Perception.Players.Type;
import Perception.Players.State;
import std;

export namespace Perception::Players::System
{
    // Builds the Players table: one row per entry of the player table with the body and the loadout of its live biped (D30, D93).
    class PlayersService
    {
    private:
        using Log = Common::Log::System::Log;

        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using ObjectTable = Blam::Runtime::Object::Type::Alive::ObjectTable;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using AlivePlayer = Blam::Runtime::Player::Type::Alive::AlivePlayer;
        using PlayerTableStore = Blam::Runtime::Player::State::PlayerTableStore;

        using AliveGame = Blam::Runtime::Game::Type::Alive::AliveGame;
        using GameStore = Blam::Runtime::Game::State::GameStore;

        using VitalityDetails = Perception::Details::Type::VitalityDetails;
        using VitalityDetailsStore = Perception::Details::State::VitalityDetailsStore;

        using Entities = Perception::Entities::Type::Entities;
        using EntitiesStore = Perception::Entities::State::EntitiesStore;

        using Body = Perception::Players::Type::Body;
        using Player = Perception::Players::Type::Player;
        using GroundMemory = Perception::Players::Type::GroundMemory;
        using Players = Perception::Players::Type::Players;
        using GroundMemoryStore = Perception::Players::State::GroundMemoryStore;
        using PlayersStore = Perception::Players::State::PlayersStore;

    public:
        PlayersService(Log& log, ObjectTableStore& objectStore, PlayerTableStore& playerStore,
            GameStore& gameStore, VitalityDetailsStore& vitalityDetailsStore, EntitiesStore& entitiesStore, GroundMemoryStore& groundMemoryStore,
            PlayersStore& playersStore) :
            m_Log(log), m_ObjectStore(objectStore), m_PlayerStore(playerStore), m_GameStore(gameStore),
            m_VitalityDetailsStore(vitalityDetailsStore), m_EntitiesStore(entitiesStore), m_GroundMemoryStore(groundMemoryStore),
            m_PlayersStore(playersStore) {}
        ~PlayersService() = default;

        // Publishes one player per entry of the player table, sorted by handle.
        // note: Runs after Entities of the same tick, which decides whether the biped is alive.
        // note: The custom powerup comes from the pickups of each player and the game time and powerup durations of the game store, read in
        // this same call, so it does not depend on the order of the pipeline. Without game it is None (D263).
        // note: Resolves the ground of each biped with the memory of the previous tick (D260).
        auto Update() -> void;

        // Drops the published players.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        PlayerTableStore& m_PlayerStore;
        GameStore& m_GameStore;
        VitalityDetailsStore& m_VitalityDetailsStore;
        EntitiesStore& m_EntitiesStore;
        GroundMemoryStore& m_GroundMemoryStore;
        PlayersStore& m_PlayersStore;

        // Builds the player of an entry of the player table.
        // param previous: Ground memory of the previous tick. nextMemory: Receives the entry of the biped of this tick.
        // param game: Game time and powerup durations for the custom powerup, null when the game was not captured.
        auto MakePlayer(const AlivePlayer& alive, const AliveGame* game, const ObjectTable& objectTable, const Entities& entities,
            const VitalityDetails& vitalities, const GroundMemory& previous, GroundMemory& nextMemory) const -> Player;

        // Builds the body of the live biped of a player.
        // return: Empty when the player has no live biped.
        auto MakeBody(const AlivePlayer& alive, const ObjectTable& objectTable, const Entities& entities,
            const VitalityDetails& vitalities, const GroundMemory& previous, GroundMemory& nextMemory) const -> std::optional<Body>;
    };
}