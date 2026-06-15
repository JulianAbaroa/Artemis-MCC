#pragma once

#include "Core/Types/Sources/Tables/Object/BoneMatrix/BoneMatrix.h"

#include <unordered_map>

using BoneMatrixTables = std::unordered_map<uint32_t, BoneMatrixTable>;

class State_BoneMatrices
{
public:
    const BoneMatrixTable* Get(uint32_t handle) const;
    void Set(uint32_t handle, BoneMatrixTable table);

    void Clear();

private:
    BoneMatrixTables m_BoneMatrixTables;
};