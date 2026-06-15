#include "pch.h"

#include "State_PlayerTable.h"

uintptr_t State_PlayerTable::GetBase() const
{
	return m_PlayerTableBase.load();
}

void State_PlayerTable::SetBase(uintptr_t pointer)
{
	m_PlayerTableBase.store(pointer);
}

void State_PlayerTable::AddPlayer(
	uint32_t handle, const LivePlayer& player)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_PlayerTable.emplace(handle, player);
}

void State_PlayerTable::RemovePlayer(
	uint32_t handle)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = m_PlayerTable.find(handle);
	if (it == m_PlayerTable.end()) return;
	m_PlayerTable.erase(handle);
}

void State_PlayerTable::RemovePlayerIf(
	std::function<bool(uint32_t, const LivePlayer&)> predicate)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	for (auto it = m_PlayerTable.begin(); it != m_PlayerTable.end();)
	{
		if (predicate(it->first, it->second))
		{
			it = m_PlayerTable.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void State_PlayerTable::UpdatePlayers(
	std::function<void(uint32_t, LivePlayer&)> processor)
{
	std::scoped_lock lock(m_Mutex);
	for (auto& [handle, player] : m_PlayerTable)
	{
		processor(handle, player);
	}
}

void State_PlayerTable::Publish()
{
	std::shared_ptr<const PlayerTable> snap;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		snap = std::make_shared<const PlayerTable>(m_PlayerTable);
	}
	m_pPlayerTable.store(snap, std::memory_order_release);
}

std::shared_ptr<const PlayerTable> State_PlayerTable::Acquire() const
{
	return m_pPlayerTable.load(std::memory_order_acquire);
}

void State_PlayerTable::Cleanup()
{
	m_PlayerTableBase.store(0);
	m_pPlayerTable.store(nullptr, std::memory_order_release);

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_PlayerTable.clear();
}