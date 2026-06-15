#pragma once

#include <unordered_map>
#include <cstdint>
#include <vector>

struct Classified;
struct LiveObject;
struct Obstacle;
struct Spawn;
struct Teleporter;
struct Lift;
struct Shield;
struct ObjectiveSpawn;
struct Objective;
struct Destructible;

using Classifieds = std::vector<Classified>;
using ObjectTable = std::unordered_map<uint32_t, LiveObject>;
using Obstacles = std::vector<Obstacle>;
using Spawns = std::vector<Spawn>;
using Teleporters = std::vector<Teleporter>;
using Lifts = std::vector<Lift>;
using Shields = std::vector<Shield>;
using ObjectiveSpawns = std::vector<ObjectiveSpawn>;
using Objectives = std::vector<Objective>;
using Destructibles = std::vector<Destructible>;

class State_ObjectTable;
class State_Classifier;
class State_Fixtures;
class System_Logs;

struct Sys_Fixtures_Deps
{
    State_ObjectTable& State_ObjectTable;
    State_Classifier& State_Classifier;
    State_Fixtures& State_Fixtures;
    System_Logs& System_Logs;
};

class System_Fixtures
{
public:
    System_Fixtures(Sys_Fixtures_Deps deps) : m_Deps(deps) {}
    ~System_Fixtures() = default;

    void Update();

    void Cleanup();

private:
    Sys_Fixtures_Deps m_Deps;

    void CollectObstacles(const Classifieds& classifieds,
        const ObjectTable& objects, Obstacles& obstacles);

    void CollectSpawns(const Classifieds& classifieds,
        const ObjectTable& objects, Spawns& spawns);

    void CollectTeleports(const Classifieds& classifieds,
        const ObjectTable& objects, Teleporters& teleporters);

    void CollectLifts(const Classifieds& classifieds,
        const ObjectTable& objects, Lifts& lifts);

    void CollectShields(const Classifieds& classifieds,
        const ObjectTable& objects, Shields& shields);

    void CollectObjectives(const Classifieds& classifieds,
        const ObjectTable& objects, Objectives& objectives,
        ObjectiveSpawns& objectiveSpawns);

    void CollectDestructibles(const Classifieds& classifieds,
        const ObjectTable& objects, Destructibles& destructibles);
};