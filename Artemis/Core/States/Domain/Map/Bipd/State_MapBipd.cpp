#include "pch.h"

// Header.
#include "State_MapBipd.h"

bool State_MapBipd::HasBipd(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Bipds.count(tagName) > 0;
}

const BipdObject* State_MapBipd::GetBipd(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Bipds.find(tagName);
    return it != m_Bipds.end() ? &it->second : nullptr;
}

void State_MapBipd::AddBipd(const std::string& tagName, BipdObject data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Bipds.emplace(tagName, std::move(data));
}

void State_MapBipd::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Bipds.clear();
}