#pragma once

#include "Core/Types/Domain/Graph/GraphTypes.h"
#include <unordered_map>
#include <cstdint>

class System_ObjectGraph
{
public:
	void UpdateGraph();
	void Cleanup();

private:
	void BuildNodes(std::unordered_map<uint32_t, ObjectNode>& nodes);
	void BuildPlayerTrees(const std::unordered_map<uint32_t, ObjectNode>& nodes,
		std::vector<PlayerObjectTree>& trees);

	ObjectNode* Resolve(std::unordered_map<uint32_t, ObjectNode>& nodes,
		uint32_t handle);
};