#include "pch.h"
#include "Core/States/Domain/Map/Jmad/State_MapJmad.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool State_MapJmad::HasJmad(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Jmads.count(tagName) > 0;
}

const JmadObject* State_MapJmad::GetJmad(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Jmads.find(tagName);
    return it != m_Jmads.end() ? &it->second : nullptr;
}

void State_MapJmad::AddJmad(const std::string& tagName, JmadObject data)
{
    //g_pSystem->Debug->Log("[MapJmadState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Jmads.emplace(tagName, std::move(data));
}

void State_MapJmad::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Jmads.clear();
}