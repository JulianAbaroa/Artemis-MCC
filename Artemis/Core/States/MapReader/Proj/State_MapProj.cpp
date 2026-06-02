#include "pch.h"

#include "State_MapProj.h"

bool State_MapProj::HasProj(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Projs.count(tagName) > 0;
}

const ProjObject* State_MapProj::GetProj(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Projs.find(tagName);
    return it != m_Projs.end() ? &it->second : nullptr;
}

void State_MapProj::AddProj(const std::string& tagName, ProjObject data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Projs.emplace(tagName, std::move(data));
}

void State_MapProj::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Projs.clear();
}