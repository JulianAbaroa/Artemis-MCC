#include "pch.h"
#include "Core/States/Domain/Map/Mode/MapModeState.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool MapModeState::HasMode(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Modes.count(tagName) > 0;
}

const ModeObject* MapModeState::GetMode(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Modes.find(tagName);
    return it != m_Modes.end() ? &it->second : nullptr;
}

void MapModeState::AddMode(const std::string& tagName, ModeObject data)
{
    //g_pSystem->Debug->Log("[MapModeState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Modes.emplace(tagName, std::move(data));
}

void MapModeState::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Modes.clear();
}