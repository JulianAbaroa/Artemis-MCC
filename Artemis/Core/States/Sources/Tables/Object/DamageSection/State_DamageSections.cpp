#include "pch.h"

#include "State_DamageSections.h"

const DamageSectionTable* State_DamageSections::Get(
    uint32_t handle) const
{
    auto it = m_DamageSectionTables.find(handle);
    return it != m_DamageSectionTables.end() ? &it->second : nullptr;
}

void State_DamageSections::Set(
    uint32_t handle, DamageSectionTable table)
{
    m_DamageSectionTables.emplace(handle, std::move(table));
}

void State_DamageSections::Clear()
{
    m_DamageSectionTables.clear();
}