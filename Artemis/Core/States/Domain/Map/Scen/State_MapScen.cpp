#include "pch.h"
#include "Core/States/Domain/Map/Scen/State_MapScen.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool State_MapScen::HasScen(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Scens.count(tagName) > 0;
}

const ScenObject* State_MapScen::GetScen(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Scens.find(tagName);
    return it != m_Scens.end() ? &it->second : nullptr;
}

void State_MapScen::AddScen(const std::string& tagName, ScenObject data)
{
    //g_pSystem->Debug->Log("[MapScenState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scens.emplace(tagName, std::move(data));
}

void State_MapScen::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scens.clear();
}