#pragma once

#include "Core/Types/Environment/Vitality/ObjectVitality.h"

#include <unordered_map>
#include <atomic>
#include <memory>

using Vitalities = std::unordered_map<uint32_t, ObjectVitality>;

class State_Vitality
{
public:
    void Publish(Vitalities vitalities);
    std::shared_ptr<const Vitalities> Acquire() const;

    void Cleanup();

private:
    std::atomic<std::shared_ptr<const Vitalities>> m_pVitalities;
};