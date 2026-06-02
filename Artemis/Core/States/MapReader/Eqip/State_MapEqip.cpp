#include "pch.h"

#include "State_MapEqip.h"

bool State_MapEqip::HasEqip(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Eqips.count(tagName) > 0;
}

const EqipObject* State_MapEqip::GetEqip(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Eqips.find(tagName);
    return it != m_Eqips.end() ? &it->second : nullptr;
}

void State_MapEqip::AddEqip(const std::string& tagName, EqipObject data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Eqips.emplace(tagName, std::move(data));
}

void State_MapEqip::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Eqips.clear();
}