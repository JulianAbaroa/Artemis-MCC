#pragma once

#include "Core/Types/Domain/Graph/GraphTypes.h"
#include <unordered_map>
#include <vector>

class UI_ObjectGraph
{
public:
	void Draw();
	void Cleanup();

private:
	void DrawPlayerTrees(const std::vector<PlayerObjectTree>& trees,
		const std::unordered_map<uint32_t, ObjectNode>& nodes);

	void DrawNodeHierarchy(uint32_t handle,
		const std::unordered_map<uint32_t, ObjectNode>& nodes);

	void DrawSelectedNodeDetails(const ObjectNode& node);

	uint32_t m_SelectedHandle = 0xFFFFFFFF;
	std::vector<uint32_t> m_DiscoveryOrder{};
};