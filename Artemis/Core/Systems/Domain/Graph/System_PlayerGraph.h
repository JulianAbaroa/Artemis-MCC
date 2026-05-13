#pragma once

// Types.
#include "Core/Types/Domain/Graph/ObjectNode.h"
#include "Core/Types/Domain/Graph/PlayerTree.h"

#include <unordered_map>
#include <vector>

class State_PlayerTable;
class State_ObjectGraph;
class State_PlayerGraph;

class System_PlayerGraph
{
public:
    void UpdateGraph();
    void Cleanup();

private:
    void BuildPlayerTrees(std::vector<PlayerTree>& trees);
    void BuildVehicle(PlayerTree& tree, 
        const std::unordered_map<uint32_t, ObjectNode>& nodes);
};