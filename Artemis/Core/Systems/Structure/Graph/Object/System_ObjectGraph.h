#pragma once

#include <unordered_map>
#include <cstdint>

struct ObjectNode;
using ObjectGraph = std::unordered_map<uint32_t, ObjectNode>;

class State_ObjectTable;
class State_ObjectGraph;
class System_Logs;

struct Sys_ObjectGraph_Deps
{
	State_ObjectTable& State_ObjectTable;
	State_ObjectGraph& State_ObjectGraph;
	System_Logs& System_Logs;
};

class System_ObjectGraph
{
public:
	System_ObjectGraph(Sys_ObjectGraph_Deps deps) : m_Deps(deps) {}
	~System_ObjectGraph() = default;

	void UpdateGraph();

	void Cleanup();

private:
	Sys_ObjectGraph_Deps m_Deps;

	void BuildNodes(ObjectGraph& nodes);
};