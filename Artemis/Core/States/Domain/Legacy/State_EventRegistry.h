#pragma once

#include "Core/Types/Domain/Legacy/TimelineTypes.h"
#include <unordered_map>
#include <functional>
#include <mutex>

// TODO: Rework this class.

class State_EventRegistry
{
public:
	bool IsEventRegistered(const std::wstring& templateStr) const;
	EventType GetEventType(const std::wstring& templateStr) const;

	void SetEventRegistry(std::unordered_map<std::wstring, EventInfo> newRegistry);
	std::unordered_map<std::wstring, EventInfo> GetEventRegistryCopy() const;

	void ForEachEvent(std::function<void(const std::wstring&, const EventInfo&)> callback) const;

private:
	std::unordered_map<std::wstring, EventInfo> m_EventRegistry;
	mutable std::mutex m_Mutex;
};