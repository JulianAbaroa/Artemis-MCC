#include "pch.h"

#include "State_BoneOffsets.h"

std::optional<BonesHeader> State_BoneOffsets::Get(uint32_t handle) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_BonesHeaders.find(handle);
    if (it == m_BonesHeaders.end()) return std::nullopt;
    return it->second;
}

bool State_BoneOffsets::Set(uint32_t handle, BonesHeader info)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto [it, inserted] = m_BonesHeaders.try_emplace(handle, info);
    return inserted;
}

void State_BoneOffsets::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_BonesHeaders.clear();
}