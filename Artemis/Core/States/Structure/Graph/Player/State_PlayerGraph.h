#pragma once

#include "Core/Types/Structure/Graph/Player/PlayerTree.h"

#include <atomic>
#include <memory>

using PlayerGraph = std::vector<PlayerTree>;

class State_PlayerGraph
{
public:
	void Publish(PlayerGraph trees);
	std::shared_ptr<const PlayerGraph> Acquire() const;

	void Cleanup();

private:
	std::atomic<std::shared_ptr<const PlayerGraph>> m_pPlayerGraph;
};