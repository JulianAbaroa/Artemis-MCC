#pragma once

// Types.
#include "Core/Types/Domain/Graph/PlayerTree.h"

#include <vector>
#include <atomic>
#include <mutex>

class State_PlayerGraph
{
public:
	const std::vector<PlayerTree>& GetTrees() const;
	void SetTrees(std::vector<PlayerTree>&& trees);
	bool HasChanged() const;
	void ClearChanged();

	void Cleanup();

private:
	mutable std::mutex m_Mutex;
	std::vector<PlayerTree> m_Trees;
	std::atomic<bool> m_HasChanged{ false };
};