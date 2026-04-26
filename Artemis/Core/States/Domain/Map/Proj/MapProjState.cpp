#include "pch.h"
#include "Core/States/Domain/Map/Proj/MapProjState.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool MapProjState::HasProj(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Projs.count(tagName) > 0;
}

const ProjObject* MapProjState::GetProj(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Projs.find(tagName);
    return it != m_Projs.end() ? &it->second : nullptr;
}

void MapProjState::AddProj(const std::string& tagName, ProjObject data)
{
    //g_pSystem->Debug->Log("[MapProjState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Projs.emplace(tagName, std::move(data));
}

void MapProjState::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Projs.clear();
}