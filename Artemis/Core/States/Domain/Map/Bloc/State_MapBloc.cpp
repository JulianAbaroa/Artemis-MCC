#include "pch.h"
#include "Core/States/Domain/Map/Bloc/State_MapBloc.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool State_MapBloc::HasBloc(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Blocs.count(tagName) > 0;
}

const BlocObject* State_MapBloc::GetBloc(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Blocs.find(tagName);
    return it != m_Blocs.end() ? &it->second : nullptr;
}

void State_MapBloc::AddBloc(const std::string& tagName, BlocObject data)
{
    //g_pSystem->Debug->Log("[MapBlocState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Blocs.emplace(tagName, std::move(data));
}

void State_MapBloc::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Blocs.clear();
}