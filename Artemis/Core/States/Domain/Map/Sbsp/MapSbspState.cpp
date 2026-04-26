#include "pch.h"
#include "Core/States/Domain/Map/Sbsp/MapSbspState.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool MapSbspState::HasSbsp(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Sbsps.count(tagName) > 0;
}

const SbspObject* MapSbspState::GetSbsp(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Sbsps.find(tagName);
    return it != m_Sbsps.end() ? &it->second : nullptr;
}

void MapSbspState::AddSbsp(const std::string& tagName, SbspObject data)
{
    //g_pSystem->Debug->Log("[MapSbspState] INFO:"
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Sbsps.emplace(tagName, std::move(data));
}

void MapSbspState::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Sbsps.clear();
}