#include "pch.h"

#include "State_MapVehi.h"

bool State_MapVehi::HasVehi(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Vehis.count(tagName) > 0;
}

const VehiObject* State_MapVehi::GetVehi(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Vehis.find(tagName);
    return it != m_Vehis.end() ? &it->second : nullptr;
}

void State_MapVehi::AddVehi(const std::string& tagName, VehiObject data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Vehis.emplace(tagName, std::move(data));
}

void State_MapVehi::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Vehis.clear();
}