#pragma once

#include "Core/Types/Sources/Tables/Object/BoneMatrix/BoneMatrix.h"

#include <unordered_map>
#include <optional>
#include <mutex>

using BonesHeaders = std::unordered_map<uint32_t, BonesHeader>;

class State_BoneOffsets
{
public:
    std::optional<BonesHeader> Get(uint32_t handle) const;
    bool Set(uint32_t handle, BonesHeader info);

    void Cleanup();

private:
    BonesHeaders m_BonesHeaders;
    mutable std::mutex m_Mutex;
};