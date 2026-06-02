#include "pch.h"

#include "State_MapZone.h"

bool State_MapZone::HasZone(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Zones.count(tagName) > 0;
}

const ZoneObject* State_MapZone::GetZone(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Zones.find(tagName);
    return it != m_Zones.end() ? &it->second : nullptr;
}

void State_MapZone::AddZone(const std::string& tagName, ZoneObject zone)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Zones[tagName] = std::move(zone);
}

const ZoneObject* State_MapZone::GetFirstZone() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Zones.empty()) return nullptr;
    return &m_Zones.begin()->second;
}

void State_MapZone::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Zones.clear();
}