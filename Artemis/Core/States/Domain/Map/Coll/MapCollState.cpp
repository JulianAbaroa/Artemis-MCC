#include "pch.h"
#include "Core/States/Domain/Map/Coll/MapCollState.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool MapCollState::HasColl(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Colls.count(tagName) > 0;
}

const CollObject* MapCollState::GetColl(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Colls.find(tagName);
    return it != m_Colls.end() ? &it->second : nullptr;
}

void MapCollState::AddColl(const std::string& tagName, CollObject data)
{
    //g_pSystem->Debug->Log("[MapCollState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Colls.emplace(tagName, std::move(data));
}

void MapCollState::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Colls.clear();
}