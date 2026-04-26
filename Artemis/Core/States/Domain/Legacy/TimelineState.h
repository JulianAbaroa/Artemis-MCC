#pragma once

#include "Core/Common/Types/Domain/TimelineTypes.h"
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

// TODO: Rework this class.

//class TimelineState
//{
//public:
//	void AddGameEvent(const GameEvent& event);
//
//	std::vector<GameEvent> GetTimelineCopy() const;
//	void SetTimeline(std::vector<GameEvent> newTimeline);
//	size_t GetTimelineSize() const;
//	void ClearTimeline();
//
//private:
//	// Accumulates the events of a replay during the Timeline phase.
//	std::vector<GameEvent> m_Events{};
//	mutable std::mutex m_Mutex;
//};