#include "pch.h"
#include "Core/States/Domain/Map/Weap/State_MapWeap.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool State_MapWeap::HasWeap(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Weaps.count(tagName) > 0;
}

const WeapObject* State_MapWeap::GetWeap(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Weaps.find(tagName);
    return it != m_Weaps.end() ? &it->second : nullptr;
}

void State_MapWeap::AddWeap(const std::string& tagName, WeapObject data)
{
    //g_pSystem->Debug->Log("[MapWeapState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Weaps.emplace(tagName, std::move(data));
}

void State_MapWeap::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Weaps.clear();
}