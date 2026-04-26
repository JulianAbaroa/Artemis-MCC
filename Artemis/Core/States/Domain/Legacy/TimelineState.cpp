#include "pch.h"
#include "Core/States/Domain/Legacy/TimelineState.h"

//void TimelineState::AddGameEvent(const GameEvent& event)
//{
//	std::lock_guard<std::mutex> lock(m_Mutex);
//	m_Events.push_back(event);
//}
//
//
//std::vector<GameEvent> TimelineState::GetTimelineCopy() const
//{
//	std::lock_guard<std::mutex> lock(m_Mutex);
//	return m_Events;
//}
//
//void TimelineState::SetTimeline(std::vector<GameEvent> newTimeline)
//{
//	std::lock_guard<std::mutex> lock(m_Mutex);
//	m_Events = std::move(newTimeline);
//}
//
//size_t TimelineState::GetTimelineSize() const
//{
//	std::lock_guard<std::mutex> lock(m_Mutex);
//	return m_Events.size();
//}
//
//void TimelineState::ClearTimeline()
//{
//	std::lock_guard<std::mutex> lock(m_Mutex);
//	m_Events.clear();
//}