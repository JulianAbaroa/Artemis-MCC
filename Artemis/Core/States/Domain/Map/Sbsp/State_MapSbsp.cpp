#include "pch.h"
#include "Core/States/Domain/Map/Sbsp/State_MapSbsp.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool State_MapSbsp::HasSbsp(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Sbsps.count(tagName) > 0;
}

const SbspObject* State_MapSbsp::GetSbsp(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Sbsps.find(tagName);
    return it != m_Sbsps.end() ? &it->second : nullptr;
}

void State_MapSbsp::AddSbsp(const std::string& tagName, SbspObject data)
{
    //g_pSystem->Debug->Log("[MapSbspState] INFO:"
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Sbsps.emplace(tagName, std::move(data));
}

void State_MapSbsp::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Sbsps.clear();
}