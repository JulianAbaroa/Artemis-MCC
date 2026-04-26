#pragma once

#include "Core/Common/Types/Domain/BlamTypes.h"
#include "Core/Common/Types/Domain/TimelineTypes.h"
#include <string>
#include <vector>
#include <atomic>
#include <deque>

class TimelineSystem
{
public:
	//void ProcessEngineEvent(std::wstring& templateStr, EventData* rawData);

	void Cleanup();

private:
	// std::vector<PlayerInfo> ExtractPlayers(EventData* rawData);
};