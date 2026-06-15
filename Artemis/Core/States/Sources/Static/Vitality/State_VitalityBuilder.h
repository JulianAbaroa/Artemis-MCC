#pragma once

#include "Core/Types/Sources/Static/Vitality/VitalityLayout.h"

#include <unordered_map>
#include <cassert>
#include <atomic>
#include <string>

using VitalityLayouts = std::unordered_map<std::string, VitalityLayout>;

class State_VitalityBuilder
{
public:
    bool HasLayout(const std::string& tagName) const;
    const VitalityLayout* GetLayout(const std::string& tagName) const;
    void AddLayout(const std::string& tagName, VitalityLayout layout);

    void Freeze()
    {
        m_Frozen.store(true, std::memory_order_release);
    }

    void Cleanup();

private:
    VitalityLayouts m_VitalityLayouts;
    std::atomic<bool> m_Frozen;
};