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
        using Objective = Environment::Fixtures::Type::Objective::Objective;
        using Destructible = Environment::Fixtures::Type::Destructible::Destructible;
        using Obstacles = std::vector<Obstacle>;
        using Spawns = std::vector<Spawn>;
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

        // Collects the objective items, carried or not.
        // note: Objective spawns and zones, boundaries and the other zones are Entities with ZoneDetails now (D88).
        auto CollectObjectives(const Classifieds& classifieds,
            const ObjectTable& objects, Objectives& objectives) -> void;

        auto CollectDestructibles(const Classifieds& classifieds,
            const ObjectTable& objects, Destructibles& destructibles) -> void;
    };
}