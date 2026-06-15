#pragma once

#include <unordered_map>
#include <cstdint>
#include <vector>

struct LiveObject;
struct LivePlayer;
struct ObjectNode;
struct PlayerTree;

using ObjectTable = std::unordered_map<uint32_t, LiveObject>;
using PlayerTable = std::unordered_map<uint32_t, LivePlayer>;
using ObjectGraph = std::unordered_map<uint32_t, ObjectNode>;
using PlayerGraph = std::vector<PlayerTree>;

class State_ObjectTable;
class State_PlayerTable;
class State_Classifier;
class State_ObjectGraph;
class State_PlayerGraph;
class System_Logs;

struct Sys_PlayerGraph_Deps
{
    State_ObjectTable& State_ObjectTable;
    State_PlayerTable& State_PlayerTable;
    State_Classifier& State_Classifier;
    State_ObjectGraph& State_ObjectGraph;
    State_PlayerGraph& State_PlayerGraph;
    System_Logs& System_Logs;
};

class System_PlayerGraph
{
public:
    System_PlayerGraph(Sys_PlayerGraph_Deps deps) : m_Deps(deps) {}
    ~System_PlayerGraph() = default;

    void UpdateGraph();

    void Cleanup();

private:
    Sys_PlayerGraph_Deps m_Deps;

    void BuildPlayerTrees(PlayerGraph& playerGraph,
        const PlayerTable& playerTable, 
        const ObjectTable& objectTable);

    void BuildVehicle(PlayerTree& playerTree, 
        const ObjectGraph& objectGraph,
        const ObjectTable& objectTable);
};