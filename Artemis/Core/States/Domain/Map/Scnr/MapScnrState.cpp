#include "pch.h"
#include "Core/States/Domain/Map/Scnr/MapScnrState.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool MapScnrState::HasScnr(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Scnrs.count(tagName) > 0;
}

const ScnrObject* MapScnrState::GetScnr(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Scnrs.find(tagName);
    return it != m_Scnrs.end() ? &it->second : nullptr;
}

void MapScnrState::AddScnr(const std::string& tagName, ScnrObject data)
{
    //g_pSystem->Debug->Log("[MapScnrState] INFO:"
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scnrs.emplace(tagName, std::move(data));
}

void MapScnrState::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Scnrs.clear();
}