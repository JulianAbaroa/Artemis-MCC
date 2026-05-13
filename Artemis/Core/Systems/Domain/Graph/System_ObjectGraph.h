#pragma once

// Types.
#include "Core/Types/Domain/Graph/ObjectNode.h"

#include <unordered_map>
#include <cstdint>
#include <vector>

class System_ObjectGraph
{
public:
	void UpdateGraph();
	void Cleanup();

private:
	void BuildNodes(std::unordered_map<uint32_t, ObjectNode>& nodes);
};