#pragma once

#include "Core/Types/Sources/Tables/Object/DamageSection/DamageSection.h"

#include <unordered_map>

using DamageSectionTables = std::unordered_map<uint32_t, DamageSectionTable>;

class State_DamageSections
{
public:
    const DamageSectionTable* Get(uint32_t handle) const;
    void Set(uint32_t handle, DamageSectionTable table);

    void Clear();

private:
    DamageSectionTables m_DamageSectionTables;
};