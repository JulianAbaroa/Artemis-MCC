#include "pch.h"

// Header.
#include "State_MapCtrl.h"

bool State_MapCtrl::HasCtrl(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Ctrls.count(tagName) > 0;
}

const CtrlObject* State_MapCtrl::GetCtrl(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Ctrls.find(tagName);
    return it != m_Ctrls.end() ? &it->second : nullptr;
}

void State_MapCtrl::AddCtrl(const std::string& tagName, CtrlObject data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Ctrls.emplace(tagName, std::move(data));
}

void State_MapCtrl::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Ctrls.clear();
}