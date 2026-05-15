#pragma once

// Types.
#include "Core/Types/Domain/Graph/ObjectNode.h"

#include <unordered_map>
#include <cstdint>
#include <vector>

class State_ObjectTable;
class State_ObjectGraph;
class System_Debug;

struct System_ObjectGraph_Dependencies
{
	State_ObjectTable& State_ObjectTable;
	State_ObjectGraph& State_ObjectGraph;
	System_Debug& System_Debug;
};

class System_ObjectGraph
{
public:
	System_ObjectGraph(System_ObjectGraph_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~System_ObjectGraph() = default;

	void UpdateGraph();
	void Cleanup();

private:
	System_ObjectGraph_Dependencies m_Deps;

	void BuildNodes(std::unordered_map<uint32_t, ObjectNode>& nodes);
};