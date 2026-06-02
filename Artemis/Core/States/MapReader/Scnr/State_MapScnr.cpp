#include "pch.h"

#include "State_MapScnr.h"

bool State_MapScnr::HasScnr(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Scnrs.count(tagName) > 0;
}

const ScnrObject* State_MapScnr::GetScnr(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Scnrs.find(tagName);
    return it != m_Scnrs.end() ? &it->second : nullptr;
}

void State_MapScnr::AddScnr(const std::string& tagName, ScnrObject data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scnrs.emplace(tagName, std::move(data));
}

void State_MapScnr::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scnrs.clear();
}