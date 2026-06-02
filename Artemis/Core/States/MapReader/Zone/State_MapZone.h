#pragma once

#include "Generated/Zone/ZoneObject.h"

#include <unordered_map>
#include <string>
#include <mutex>

class State_MapZone
{
public:
    bool HasZone(const std::string& tagName) const;
    const ZoneObject* GetZone(const std::string& tagName) const;
    void AddZone(const std::string& tagName, ZoneObject zone);
    const ZoneObject* GetFirstZone() const;
    void Cleanup();

private:
    std::unordered_map<std::string, ZoneObject> m_Zones;
    mutable std::mutex m_Mutex;
};