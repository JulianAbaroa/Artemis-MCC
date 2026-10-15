export module Environment.Players.System;

import Service.Logs.System;
import Tables.Object.Type;
import Tables.Object.State;
import Tables.Player.Type;
import Tables.Player.State;
import Environment.Details.Type;
import Environment.Details.State;
import Environment.Entities.Type;
import Environment.Entities.State;
import Environment.Players.Type;
import Environment.Players.State;
import std;

export namespace Environment::Players::System
{
    // Builds the Players table: one row per entry of the player table with the body and the loadout of its live biped (D30, D93).
    class PlayersService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AliveObject = Tables::Object::Type::Alive::Object;
        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;

        using AlivePlayer = Tables::Player::Type::Alive::AlivePlayer;
        using PlayerTableStore = Tables::Player::State::PlayerTableStore;

        using VitalityDetails = Environment::Details::Type::VitalityDetails;
        using VitalityDetailsStore = Environment::Details::State::VitalityDetailsStore;

        using Entities = Environment::Entities::Type::Entities;
        using EntitiesStore = Environment::Entities::State::EntitiesStore;

        using Body = Environment::Players::Type::Body;
        using Player = Environment::Players::Type::Player;
        using Players = Environment::Players::Type::Players;
        using PlayersStore = Environment::Players::State::PlayersStore;

    public:
        PlayersService(LogsService& logsService, ObjectTableStore& objectStore, PlayerTableStore& playerStore,
            VitalityDetailsStore& vitalityDetailsStore, EntitiesStore& entitiesStore, PlayersStore& playersStore) :
            m_LogsService(logsService), m_ObjectStore(objectStore), m_PlayerStore(playerStore),
            m_VitalityDetailsStore(vitalityDetailsStore), m_EntitiesStore(entitiesStore), m_PlayersStore(playersStore) {}
        ~PlayersService() = default;

        // Publishes one player per entry of the player table, sorted by handle.
        // note: Runs after Entities of the same tick, which decides whether the biped is alive.
        auto Update() -> void;

        // Drops the published players.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
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