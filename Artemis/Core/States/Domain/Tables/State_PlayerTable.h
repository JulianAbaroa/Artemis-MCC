#pragma once

#include "Core/Types/Domain/Tables/PlayerTypes.h"
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
	const std::unordered_map<uint32_t, LivePlayer>& GetAllPlayers() const;
	void AddPlayer(uint32_t handle, const LivePlayer& player);
	std::optional<LivePlayer> RemovePlayer(uint32_t handle);
	void RemovePlayerIf(std::function<bool(uint32_t, const LivePlayer&)> predicate);
	void UpdatePlayers(std::function<void(uint32_t, LivePlayer&)> processor);
	const uint32_t GetPlayerHandleByName(std::string name) const;

	bool HasMapChanged() const;
	void SetMapChanged(bool value);

	uintptr_t GetPlayerTableBase() const;
	void SetPlayerTableBase(uintptr_t pointer);

	void Cleanup();

private:
	std::unordered_map<uint32_t, LivePlayer> m_Players;
	mutable std::mutex m_Mutex;

	std::atomic<bool> m_HasMapChanged{ false };
	std::atomic<uintptr_t> m_PlayerTableBase{ 0 };
};