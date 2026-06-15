#include "pch.h"

#include "State_Logs.h"

void State_Logs::PushBack(LogEntry entry)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Logs.push_back(std::move(entry));
}

void State_Logs::TrimToSize(int size)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Logs.size() > size)
	{
		m_Logs.pop_front();
	}
}

void State_Logs::ForEachLog(std::function<void(
	const LogEntry&)> callback) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (const auto& log : m_Logs)
	{
		callback(log);
	}
}

void State_Logs::ClearLogs()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Logs.clear();
}

int State_Logs::GetMaxCapacity() const 
{ 
	return m_MaxCapacity.load(); 
}

LogEntry State_Logs::GetLogAt(size_t index) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (index >= m_Logs.size()) return {};
	return m_Logs[index];
}

size_t State_Logs::GetTotalLogs() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Logs.size();
}

void State_Logs::RemoveIf(std::function<bool(const LogEntry&)> predicate)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	m_Logs.erase(std::remove_if(m_Logs.begin(), 
		m_Logs.end(), predicate), m_Logs.end());
}