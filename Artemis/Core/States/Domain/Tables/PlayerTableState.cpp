#include "pch.h"
#include "Core/States/Domain/Tables/PlayerTableState.h"

const LivePlayer* PlayerTableState::GetPlayer(uint32_t handle) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = m_Players.find(handle);
	return it != m_Players.end() ? &it->second : nullptr;
}

const std::unordered_map<uint32_t, LivePlayer>& PlayerTableState::GetAllPlayers() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Players;
}

void PlayerTableState::AddPlayer(uint32_t handle, const LivePlayer& player)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Players[handle] = player;
	m_HasMapChanged.store(true);
}

std::optional<LivePlayer> PlayerTableState::RemovePlayer(uint32_t handle)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	auto it = m_Players.find(handle);
	if (it != m_Players.end())
	{
		LivePlayer removedCopy = std::move(it->second);
		m_Players.erase(handle);
		m_HasMapChanged.store(true);
		return removedCopy;
	}

	return std::nullopt;
}

void PlayerTableState::RemovePlayerIf(std::function<bool(uint32_t, const LivePlayer&)> predicate)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	
	bool changed = false;
	for (auto it = m_Players.begin(); it != m_Players.end(); )
	{
		if (predicate(it->first, it->second))
		{
			it = m_Players.erase(it);
			changed = true;
		}
		else
		{
			++it;
		}
	}

	if (changed)
	{
		m_HasMapChanged.store(true);
	}
}

void PlayerTableState::UpdatePlayers(std::function<void(uint32_t, LivePlayer&)> processor)
{
	std::scoped_lock lock(m_Mutex);
	for (auto& [handle, player] : m_Players)
	{
		processor(handle, player);
	}
	m_HasMapChanged.store(true);
}

const uint32_t PlayerTableState::GetPlayerHandleByName(std::string name) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	for (const auto& [handle, player] : m_Players)
	{
		if (player.Gamertag == name)
		{
			return handle;
		}
	}

	return 0;
}

bool PlayerTableState::HasMapChanged() const { return m_HasMapChanged.load(); }
void PlayerTableState::SetMapChanged(bool value) { m_HasMapChanged.store(value); }

uintptr_t PlayerTableState::GetPlayerTableBase() const { return m_PlayerTableBase.load(); }
void PlayerTableState::SetPlayerTableBase(uintptr_t pointer) { m_PlayerTableBase.store(pointer); }

void PlayerTableState::Cleanup()
{
	m_HasMapChanged.store(false);
	m_PlayerTableBase.store(0);

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Players.clear();
}