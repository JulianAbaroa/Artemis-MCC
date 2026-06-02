#include "pch.h"

#include "State_MapMach.h"

bool State_MapMach::HasMach(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Machs.count(tagName) > 0;
}

const MachObject* State_MapMach::GetMach(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Machs.find(tagName);
    return it != m_Machs.end() ? &it->second : nullptr;
}

void State_MapMach::AddMach(const std::string& tagName, MachObject data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Machs.emplace(tagName, std::move(data));
}

void State_MapMach::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Machs.clear();
}