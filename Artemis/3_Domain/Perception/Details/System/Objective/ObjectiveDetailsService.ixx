export module Perception.Details.System:ObjectiveDetails;

import Common.Log.System;
import Common.Math.Type;
import Common.Team.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Blam.Runtime.Game.Type;
import Blam.Runtime.Game.State;
import Perception.Classifier.State;
import Perception.Entities.State;
import Perception.Players.Type;
import Perception.Players.State;
import Perception.Details.Type;
import Perception.Details.State;
import std;

export namespace Perception::Details::System
{
    // Builds the ObjectiveDetails: the state of every flag and the owner of every flag stand in the round (A14).
    class ObjectiveDetailsService
    {
    private:
        using Log = Common::Log::System::Log;

        using Vec3 = Common::Math::Type::Vec3;
        using Team = Common::Team::Type::Team;
        using AliveGame = Blam::Runtime::Game::Type::Alive::AliveGame;
        using AliveMegaloObject = Blam::Runtime::Game::Type::Alive::AliveMegaloObject;
        using Weapon = Blam::Runtime::Object::Type::Weapon::Weapon;
        using ObjectiveDetail = Perception::Details::Type::ObjectiveDetail;
        using ObjectiveDetails = Perception::Details::Type::ObjectiveDetails;
        using Players = Perception::Players::Type::Players;

        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using GameStore = Blam::Runtime::Game::State::GameStore;

        using ClassifierStore = Perception::Classifier::State::ClassifierStore;
        using EntitiesStore = Perception::Entities::State::EntitiesStore;
        using PlayersStore = Perception::Players::State::PlayersStore;

        using ZoneDetailsStore = Perception::Details::State::ZoneDetailsStore;
        using ObjectiveDetailsStore = Perception::Details::State::ObjectiveDetailsStore;

    public:
        ObjectiveDetailsService(Log& log, ObjectTableStore& objectStore, GameStore& gameStore, ClassifierStore& classifierStore,
            EntitiesStore& entitiesStore, PlayersStore& playersStore, ZoneDetailsStore& zoneDetailsStore,
            ObjectiveDetailsStore& objectiveDetailsStore) :
            m_Log(log), m_ObjectStore(objectStore), m_GameStore(gameStore), m_ClassifierStore(classifierStore),
            m_EntitiesStore(entitiesStore), m_PlayersStore(playersStore), m_ZoneDetailsStore(zoneDetailsStore),
            m_ObjectiveDetailsStore(objectiveDetailsStore) {}
        ~ObjectiveDetailsService() = default;

        // Publishes the flags and stands of the tick.
        // note: Runs after the Players of the same tick, because the carrier and the allies are bipeds of players. The Megalo state of a
        // flag is looked up by its handle, never by element. Without a usable Megalo state a flag is Home when it stands within 1.0 of its
        // stand and Dropped anywhere else.
        auto Update() -> void;

        // Drops the published details.
        auto Cleanup() -> void;

        // return: The player of each live biped of the players, by biped handle. Empty without players.
        static auto MapPlayersOfBipeds(const Players* players) -> std::unordered_map<std::uint32_t, std::uint32_t>;

        // Builds the detail of one flag from what the engine says about it, the same way for the full tick and for the flag pass of the
        // ticks without a pipeline (Q101).
        // param baseTeam: Team of the multiplayer block of the flag. position: World origin of the flag.
        // param element: Megalo element of the flag, null when it has none. game: Null before the game globals were read.
        // param stands: The stands of the round, with their position. playerOfBiped: See MapPlayersOfBipeds.
        // return: The detail without AlliesInZone: it needs the zone of the flag, which the caller knows.
        static auto MakeFlag(std::uint32_t handle, Team baseTeam, const Vec3& position, const Weapon& weapon, const AliveMegaloObject* element,
            const AliveGame* game, const ObjectiveDetails& stands, const std::unordered_map<std::uint32_t, std::uint32_t>& playerOfBiped) -> ObjectiveDetail;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        GameStore& m_GameStore;
        ClassifierStore& m_ClassifierStore;
        EntitiesStore& m_EntitiesStore;
        PlayersStore& m_PlayersStore;
        ZoneDetailsStore& m_ZoneDetailsStore;
        ObjectiveDetailsStore& m_ObjectiveDetailsStore;
    };
}