#pragma once

// Types.
#include "Core/Types/Domain/Graph/PlayerTree.h"

#include <vector>
#include <atomic>
#include <mutex>

class State_PlayerGraph
{
public:
	bool HasChanged() const;
	const std::vector<PlayerTree>& GetTrees() const;
	void SetTrees(std::vector<PlayerTree>&& trees);

	void ClearChanged();

	void Cleanup();

private:
	std::atomic<bool> m_HasChanged{ false };

	std::vector<PlayerTree> m_Trees;
	mutable std::mutex m_Mutex;
};