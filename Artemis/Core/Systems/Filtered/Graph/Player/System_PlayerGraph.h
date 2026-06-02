#pragma once

#include <unordered_map>
#include <vector>

struct ObjectNode;
struct PlayerTree;

class State_ObjectTable;
class State_PlayerTable;
class State_Classifier;
class State_ObjectGraph;
class State_PlayerGraph;
class System_Logs;

struct System_PlayerGraph_Dependencies
{
    State_ObjectTable& State_ObjectTable;
    State_PlayerTable& State_PlayerTable;
    State_Classifier& State_Classification;
    State_ObjectGraph& State_ObjectGraph;
    State_PlayerGraph& State_PlayerGraph;
    System_Logs& System_Logs;
};

class System_PlayerGraph
{
public:
    System_PlayerGraph(System_PlayerGraph_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~System_PlayerGraph() = default;

    void UpdateGraph();
    void Cleanup();

private:
    System_PlayerGraph_Dependencies m_Deps;

    void BuildPlayerTrees(std::vector<PlayerTree>& trees);
    void BuildVehicle(PlayerTree& tree, 
        const std::unordered_map<uint32_t, ObjectNode>& nodes);
};