#include "pch.h"
#include "Core/States/Domain/Tables/State_PlayerTable.h"

const LivePlayer* State_PlayerTable::GetPlayer(uint32_t handle) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = m_Players.find(handle);
	return it != m_Players.end() ? &it->second : nullptr;
}

const std::unordered_map<uint32_t, LivePlayer>& State_PlayerTable::GetAllPlayers() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Players;
}

void State_PlayerTable::AddPlayer(uint32_t handle, const LivePlayer& player)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Players[handle] = player;
	m_HasMapChanged.store(true);
}

std::optional<LivePlayer> State_PlayerTable::RemovePlayer(uint32_t handle)
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

void State_PlayerTable::RemovePlayerIf(std::function<bool(uint32_t, const LivePlayer&)> predicate)
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

void State_PlayerTable::UpdatePlayers(std::function<void(uint32_t, LivePlayer&)> processor)
{
	std::scoped_lock lock(m_Mutex);
	for (auto& [handle, player] : m_Players)
	{
		processor(handle, player);
	}
	m_HasMapChanged.store(true);
}

const uint32_t State_PlayerTable::GetPlayerHandleByName(std::string name) const
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

bool State_PlayerTable::HasMapChanged() const { return m_HasMapChanged.load(); }
void State_PlayerTable::SetMapChanged(bool value) { m_HasMapChanged.store(value); }

uintptr_t State_PlayerTable::GetPlayerTableBase() const { return m_PlayerTableBase.load(); }
void State_PlayerTable::SetPlayerTableBase(uintptr_t pointer) { m_PlayerTableBase.store(pointer); }

void State_PlayerTable::Cleanup()
{
	m_HasMapChanged.store(false);
	m_PlayerTableBase.store(0);

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Players.clear();
}