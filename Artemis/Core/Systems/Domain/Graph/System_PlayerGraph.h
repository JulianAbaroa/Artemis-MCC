#pragma once

// Types.
#include "Core/Types/Domain/Graph/ObjectNode.h"
#include "Core/Types/Domain/Graph/PlayerTree.h"

#include <unordered_map>
#include <vector>

class State_ObjectTable;
class State_PlayerTable;
class State_Classification;
class State_ObjectGraph;
class State_PlayerGraph;
class System_Debug;

struct System_PlayerGraph_Dependencies
{
    State_ObjectTable& State_ObjectTable;
    State_PlayerTable& State_PlayerTable;
    State_Classification& State_Classification;
    State_ObjectGraph& State_ObjectGraph;
    State_PlayerGraph& State_PlayerGraph;
    System_Debug& System_Debug;
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