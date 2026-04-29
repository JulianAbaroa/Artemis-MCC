#include "pch.h"
#include "Core/States/Domain/Map/Vehi/State_MapVehi.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool State_MapVehi::HasVehi(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Vehis.count(tagName) > 0;
}

const VehiObject* State_MapVehi::GetVehi(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Vehis.find(tagName);
    return it != m_Vehis.end() ? &it->second : nullptr;
}

void State_MapVehi::AddVehi(const std::string& tagName, VehiObject data)
{
    //g_pSystem->Debug->Log("[MapVehiState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Vehis.emplace(tagName, std::move(data));
}

void State_MapVehi::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Vehis.clear();
}