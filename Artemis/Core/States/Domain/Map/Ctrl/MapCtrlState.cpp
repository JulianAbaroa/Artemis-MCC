#include "pch.h"
#include "Core/States/Domain/Map/Ctrl/MapCtrlState.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool MapCtrlState::HasCtrl(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Ctrls.count(tagName) > 0;
}

const CtrlObject* MapCtrlState::GetCtrl(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Ctrls.find(tagName);
    return it != m_Ctrls.end() ? &it->second : nullptr;
}

void MapCtrlState::AddCtrl(const std::string& tagName, CtrlObject data)
{
    //g_pSystem->Debug->Log("[MapCtrlState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Ctrls.emplace(tagName, std::move(data));
}

void MapCtrlState::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Ctrls.clear();
}