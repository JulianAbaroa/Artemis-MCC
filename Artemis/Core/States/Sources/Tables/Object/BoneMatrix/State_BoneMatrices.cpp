#include "pch.h"

#include "State_BoneMatrices.h"

const BoneMatrixTable* State_BoneMatrices::Get(uint32_t handle) const
{
    auto it = m_BoneMatrixTables.find(handle);
    return it != m_BoneMatrixTables.end() ? &it->second : nullptr;
}

void State_BoneMatrices::Set(uint32_t handle, BoneMatrixTable table)
{
    m_BoneMatrixTables.emplace(handle, std::move(table));
}

void State_BoneMatrices::Clear() 
{ 
    m_BoneMatrixTables.clear(); 
}