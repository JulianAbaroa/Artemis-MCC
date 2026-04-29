#include "pch.h"
#include "Core/States/Domain/Map/Proj/State_MapProj.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

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
    //g_pSystem->Debug->Log("[MapProjState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Projs.emplace(tagName, std::move(data));
}

void State_MapProj::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Projs.clear();
}