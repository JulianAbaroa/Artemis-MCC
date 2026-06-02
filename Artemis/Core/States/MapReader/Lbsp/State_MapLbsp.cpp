#include "pch.h"

#include "State_MapLbsp.h"

bool State_MapLbsp::HasLbsp(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Lbsps.count(tagName) > 0;
}

const LbspObject* State_MapLbsp::GetLbsp(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Lbsps.find(tagName);
    return it != m_Lbsps.end() ? &it->second : nullptr;
}

void State_MapLbsp::AddLbsp(const std::string& tagName, LbspObject zone)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Lbsps[tagName] = std::move(zone);
}

const LbspObject* State_MapLbsp::GetFirstLbsp() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Lbsps.empty()) return nullptr;
    return &m_Lbsps.begin()->second;
}

const LbspObject* State_MapLbsp::GetLbspBySbspIndex(int32_t sbspIndex) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& [name, lbsp] : m_Lbsps)
    {
        if (lbsp.Data.BspReferenceIndex == sbspIndex)
            return &lbsp;
    }
    return nullptr;
}

const std::unordered_map<std::string, LbspObject> State_MapLbsp::GetAllLbsp() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Lbsps;
}

void State_MapLbsp::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Lbsps.clear();
}