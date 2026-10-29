export module Perception.Players.System;

import Common.Log.System;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Blam.Runtime.Player.Type;
import Blam.Runtime.Player.State;
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

        using VitalityDetails = Perception::Details::Type::VitalityDetails;
        using VitalityDetailsStore = Perception::Details::State::VitalityDetailsStore;

        using Entities = Perception::Entities::Type::Entities;
        using EntitiesStore = Perception::Entities::State::EntitiesStore;

        using Body = Perception::Players::Type::Body;
        using Player = Perception::Players::Type::Player;
        using Players = Perception::Players::Type::Players;
        using PlayersStore = Perception::Players::State::PlayersStore;

    public:
        PlayersService(Log& log, ObjectTableStore& objectStore, PlayerTableStore& playerStore,
            VitalityDetailsStore& vitalityDetailsStore, EntitiesStore& entitiesStore, PlayersStore& playersStore) :
            m_Log(log), m_ObjectStore(objectStore), m_PlayerStore(playerStore),
            m_VitalityDetailsStore(vitalityDetailsStore), m_EntitiesStore(entitiesStore), m_PlayersStore(playersStore) {}
        ~PlayersService() = default;

        // Publishes one player per entry of the player table, sorted by handle.
        // note: Runs after Entities of the same tick, which decides whether the biped is alive.
        auto Update() -> void;

        // Drops the published players.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        PlayerTableStore& m_PlayerStore;
        VitalityDetailsStore& m_VitalityDetailsStore;
        EntitiesStore& m_EntitiesStore;
        PlayersStore& m_PlayersStore;

        // Builds the player of an entry of the player table.
        auto MakePlayer(const AlivePlayer& alive, const ObjectTable& objectTable, const Entities& entities,
            const VitalityDetails& vitalities) const -> Player;

        // Builds the body of the live biped of a player.
        // return: Empty when the player has no live biped.
        auto MakeBody(const AlivePlayer& alive, const ObjectTable& objectTable, const Entities& entities,
            const VitalityDetails& vitalities) const -> std::optional<Body>;
    };
}