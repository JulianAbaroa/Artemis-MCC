#pragma once

#include <unordered_map>
#include <vector>

struct ObjectNode;
struct PlayerTree;

class State_ObjectTable;
class State_PlayerTable;
class State_ObjectGraph;
class State_PlayerGraph;
class System_Debug;

struct UI_ObjectGraph_Dependencies
{
	State_ObjectTable& State_ObjectTable;
	State_PlayerTable& State_PlayerTable;
	State_ObjectGraph& State_ObjectGraph;
	State_PlayerGraph& State_PlayerGraph;
	System_Debug& System_Debug;
};

class UI_ObjectGraph
{
public:
	UI_ObjectGraph(UI_ObjectGraph_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~UI_ObjectGraph() = default;

	void Draw();
	void Cleanup();

private:
	UI_ObjectGraph_Dependencies m_Deps;

	void DrawPlayerTrees(const std::vector<PlayerTree>& trees,
		const std::unordered_map<uint32_t, ObjectNode>& nodes);

	void DrawNodeHierarchy(uint32_t handle,
		const std::unordered_map<uint32_t, ObjectNode>& nodes);

	void DrawSelectedNodeDetails(const ObjectNode& node);

	uint32_t m_SelectedHandle = 0xFFFFFFFF;
	std::vector<uint32_t> m_DiscoveryOrder{};
};