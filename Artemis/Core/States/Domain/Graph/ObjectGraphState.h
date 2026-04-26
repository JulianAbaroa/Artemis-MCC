#pragma once

#include "Core/Common/Types/Domain/Graph/GraphTypes.h"
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <mutex>

class ObjectGraphState
{
public:
	const std::unordered_map<uint32_t, ObjectNode>& GetNodes() const;
	const std::vector<PlayerObjectTree>& GetPlayerTrees() const;

	void SetNodes(std::unordered_map<uint32_t, ObjectNode> nodes);
	void SetPlayerTrees(std::vector<PlayerObjectTree> trees);

	void Cleanup();

private:
	std::unordered_map<uint32_t, ObjectNode> m_Nodes;
	std::vector<PlayerObjectTree> m_PlayerTrees;
	mutable std::mutex m_Mutex;
};