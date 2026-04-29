#include "pch.h"
#include "Core/States/Domain/Legacy/State_EventRegistry.h"

bool State_EventRegistry::IsEventRegistered(const std::wstring& templateStr) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_EventRegistry.find(templateStr) != m_EventRegistry.end();
}

EventType State_EventRegistry::GetEventType(const std::wstring& templateStr) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	
	auto it = m_EventRegistry.find(templateStr);
	if (it != m_EventRegistry.end())
	{
		return it->second.Type;
	}

	return EventType::Unknown;
}


void State_EventRegistry::SetEventRegistry(std::unordered_map<std::wstring, EventInfo> newRegistry)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_EventRegistry = std::move(newRegistry);
}


std::unordered_map<std::wstring, EventInfo> State_EventRegistry::GetEventRegistryCopy() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_EventRegistry;
}

void State_EventRegistry::ForEachEvent(std::function<void(const std::wstring&, const EventInfo&)> callback) const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (const auto& [name, info] : m_EventRegistry)
	{
		callback(name, info);
	}
}