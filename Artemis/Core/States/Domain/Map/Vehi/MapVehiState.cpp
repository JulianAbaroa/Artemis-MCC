#include "pch.h"
#include "Core/States/Domain/Map/Vehi/MapVehiState.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool MapVehiState::HasVehi(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Vehis.count(tagName) > 0;
}

const VehiObject* MapVehiState::GetVehi(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Vehis.find(tagName);
    return it != m_Vehis.end() ? &it->second : nullptr;
}

void MapVehiState::AddVehi(const std::string& tagName, VehiObject data)
{
    //g_pSystem->Debug->Log("[MapVehiState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Vehis.emplace(tagName, std::move(data));
}

void MapVehiState::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Vehis.clear();
}