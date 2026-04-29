#include "pch.h"
#include "Core/States/Domain/Map/Scnr/State_MapScnr.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

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
    //g_pSystem->Debug->Log("[MapScnrState] INFO:"
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scnrs.emplace(tagName, std::move(data));
}

void State_MapScnr::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scnrs.clear();
}