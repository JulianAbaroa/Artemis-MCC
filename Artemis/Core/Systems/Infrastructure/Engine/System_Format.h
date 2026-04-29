#pragma once

#include "Core/Types/Domain/Legacy/TimelineTypes.h"
#include "Core/Types/Infrastructure/UITypes.h"
#include <unordered_map>
#include <string>

class System_Format
{
public:
    std::string ToTimestamp(float totalSeconds);

    std::string WStringToString(const std::wstring& wstr);
    std::wstring StringToWString(const std::string& str);

    std::string EventTypeToString(EventType type);
    const char* GetEventClassName(EventClass eventClass);
    //const std::unordered_map<EventType, EventMetadata>& GetEventMetadata();
};