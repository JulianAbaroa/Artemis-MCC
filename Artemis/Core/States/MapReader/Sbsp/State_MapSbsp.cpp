#include "pch.h"

#include "State_MapSbsp.h"

bool State_MapSbsp::HasSbsp(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Sbsps.count(tagName) > 0;
}

const SbspObject* State_MapSbsp::GetSbsp(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Sbsps.find(tagName);
    return it != m_Sbsps.end() ? &it->second : nullptr;
}

void State_MapSbsp::AddSbsp(const std::string& tagName, SbspObject data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Sbsps.emplace(tagName, std::move(data));
}

const SbspObject* State_MapSbsp::GetFirstSbsp() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Sbsps.empty()) return nullptr;
    return &m_Sbsps.begin()->second;
}

void State_MapSbsp::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Sbsps.clear();
}