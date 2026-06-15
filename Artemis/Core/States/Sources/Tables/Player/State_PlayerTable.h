#pragma once

#include "Core/Types/Sources/Tables/Player/LivePlayer.h"

#include <unordered_map>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>

using PlayerTable = std::unordered_map<uint32_t, LivePlayer>;

class State_PlayerTable
{
public:
	uintptr_t GetBase() const;
	void SetBase(uintptr_t pointer);

	void AddPlayer(uint32_t handle, const LivePlayer& player);
	void RemovePlayer(uint32_t handle);
	void RemovePlayerIf(std::function<bool(uint32_t, const LivePlayer&)> predicate);

	void UpdatePlayers(std::function<void(uint32_t, LivePlayer&)> processor);

	void Publish();
	std::shared_ptr<const PlayerTable> Acquire() const;

	void Cleanup();

private:
	std::atomic<uintptr_t> m_PlayerTableBase{ 0 };
	std::atomic<std::shared_ptr<const PlayerTable>> m_pPlayerTable;

	PlayerTable m_PlayerTable;
	mutable std::mutex m_Mutex;
};