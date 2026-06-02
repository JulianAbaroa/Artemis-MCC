#pragma once

#include <unordered_map>
#include <cstdint>
#include <vector>

struct ObjectNode;

class State_ObjectTable;
class State_ObjectGraph;
class System_Logs;

struct System_ObjectGraph_Dependencies
{
	State_ObjectTable& State_ObjectTable;
	State_ObjectGraph& State_ObjectGraph;
	System_Logs& System_Logs;
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