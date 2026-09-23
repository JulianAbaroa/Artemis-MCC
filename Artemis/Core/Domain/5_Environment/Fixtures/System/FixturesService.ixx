export module Environment.Fixtures.System;

import Service.Logs.System;
import Tables.Object.Type;
import Tables.Object.State;
import Relations.Classifier.Type;
import Relations.Classifier.State;
import Environment.Fixtures.Type;
import Environment.Fixtures.State;
import std;

export namespace Environment::Fixtures::System
{
    class FixturesService
    {
    private:
        using AliveObject = Tables::Object::Type::Alive::Object;
        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using Classified = Relations::Classifier::Type::Classified;
        using Classifieds = std::vector<Classified>;
        using Obstacle = Environment::Fixtures::Type::Obstacle::Obstacle;
        using Spawn = Environment::Fixtures::Type::Spawn::Spawn;
        using Teleport = Environment::Fixtures::Type::Teleport::Teleport;
        using Lift = Environment::Fixtures::Type::Lift::Lift;
        using Shield = Environment::Fixtures::Type::Shield::Shield;
        using ObjectiveSpawn = Environment::Fixtures::Type::ObjectiveSpawn::ObjectiveSpawn;
        using Objective = Environment::Fixtures::Type::Objective::Objective;
        using Destructible = Environment::Fixtures::Type::Destructible::Destructible;
        using Obstacles = std::vector<Obstacle>;
        using Spawns = std::vector<Spawn>;
        using Teleporters = std::vector<Teleport>;
        using Lifts = std::vector<Lift>;
        using Shields = std::vector<Shield>;
        using ObjectiveSpawns = std::vector<ObjectiveSpawn>;
        using Objectives = std::vector<Objective>;
        using Destructibles = std::vector<Destructible>;

        using LogsService = Service::Logs::System::LogsService;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;
        using FixturesStore = Environment::Fixtures::State::FixturesStore;

    public:
        FixturesService(LogsService& logsService, ObjectTableStore& objectStore,
        ClassifierStore& classifierStore, FixturesStore& fixturesStore) : 
        m_LogsService(logsService), m_ObjectStore(objectStore),
        m_ClassifierStore(classifierStore), m_FixturesStore(fixturesStore) {}
        ~FixturesService() = default;

        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        ObjectTableStore& m_ObjectStore;
        ClassifierStore& m_ClassifierStore;
        FixturesStore& m_FixturesStore;

        auto CollectObstacles(const Classifieds& classifieds,
            const ObjectTable& objects, Obstacles& obstacles) -> void;

        auto CollectSpawns(const Classifieds& classifieds,
            const ObjectTable& objects, Spawns& spawns) -> void;

        auto CollectTeleports(const Classifieds& classifieds,
            const ObjectTable& objects, Teleporters& teleporters) -> void;

        auto CollectLifts(const Classifieds& classifieds,
            const ObjectTable& objects, Lifts& lifts) -> void;

        auto CollectShields(const Classifieds& classifieds,
            const ObjectTable& objects, Shields& shields) -> void;

        auto CollectObjectives(const Classifieds& classifieds,
            const ObjectTable& objects, Objectives& objectives,
            ObjectiveSpawns& objectiveSpawns) -> void;

        auto CollectDestructibles(const Classifieds& classifieds,
            const ObjectTable& objects, Destructibles& destructibles) -> void;
    };
}