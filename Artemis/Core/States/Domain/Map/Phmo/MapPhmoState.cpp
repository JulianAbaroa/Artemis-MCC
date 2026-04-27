#include "pch.h"
#include "Core/States/Domain/Map/Phmo/MapPhmoState.h"

//#include "Core/Systems/CoreSystem.h"
//#include "Core/Systems/Interface/DebugSystem.h"

bool MapPhmoState::HasPhmo(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Phmos.count(tagName) > 0;
}

const PhmoObject* MapPhmoState::GetPhmo(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Phmos.find(tagName);
    return it != m_Phmos.end() ? &it->second : nullptr;
}

void MapPhmoState::AddPhmo(const std::string& tagName, PhmoObject data)
{
    //g_pSystem->Debug->Log("[MapPhmoState] INFO:" 
    //    " Added: %s | RigidBodies=%zu | Nodes=%zu | Materials=%zu",
    //    tagName.c_str(), data.RigidBodies.size(),
    //    data.Nodes.size(), data.Materials.size());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Phmos.emplace(tagName, std::move(data));
}

size_t MapPhmoState::GetSize() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Phmos.size();
}

std::vector<std::string> MapPhmoState::GetTagNames()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    std::vector<std::string> names;
    names.reserve(m_Phmos.size());

    for (const auto& [name, object] : m_Phmos) names.push_back(name);

    return names;
}

void MapPhmoState::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Phmos.clear();
}