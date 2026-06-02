#pragma once

#include "Core/Types/Player/LivePlayer.h"

#include <unordered_map>
#include <functional>
#include <optional>
#include <cstdint>
#include <atomic>
#include <mutex>

class State_PlayerTable
{
public:
	const LivePlayer* GetPlayer(uint32_t handle) const;
	std::optional<LivePlayer> CopyLivePlayer(uint32_t handle);

	void AddPlayer(uint32_t handle, const LivePlayer& player);
	std::optional<LivePlayer> RemovePlayer(uint32_t handle);
	void RemovePlayerIf(std::function<bool(uint32_t, const LivePlayer&)> predicate);

	const std::unordered_map<uint32_t, LivePlayer> GetPlayerTable() const;

	const uint32_t GetPlayerHandleByName(std::string name) const;

	void UpdatePlayers(std::function<void(uint32_t, LivePlayer&)> processor);

	bool HasMapChanged() const;
	void SetMapChanged(bool value);

	uintptr_t GetBase() const;
	void SetBase(uintptr_t pointer);

	void Cleanup();

private:
	std::atomic<bool> m_HasMapChanged{ false };
	std::atomic<uintptr_t> m_PlayerTableBase{ 0 };

	std::unordered_map<uint32_t, LivePlayer> m_Players;
	mutable std::mutex m_Mutex;
};