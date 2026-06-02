#include "pch.h"

#include "State_MapSldt.h"

bool State_MapSldt::HasSldt(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Sldts.count(tagName) > 0;
}

const SldtObject* State_MapSldt::GetSldt(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Sldts.find(tagName);
    return it != m_Sldts.end() ? &it->second : nullptr;
}

void State_MapSldt::AddSldt(const std::string& tagName, SldtObject zone)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Sldts[tagName] = std::move(zone);
}

void State_MapSldt::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Sldts.clear();
}