#pragma once

#include "Core/Types/Domain/BlamTypes.h"
#include "Core/Types/Domain/Legacy/TimelineTypes.h"
#include <string>
#include <vector>
#include <atomic>
#include <deque>

class System_Timeline
{
public:
	//void ProcessEngineEvent(std::wstring& templateStr, EventData* rawData);

	void Cleanup();

private:
	// std::vector<PlayerInfo> ExtractPlayers(EventData* rawData);
};