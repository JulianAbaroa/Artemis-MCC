#include "pch.h"

#include "State_VitalityBuilder.h"

bool State_VitalityBuilder::HasLayout(const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    return m_VitalityLayouts.find(tagName) != m_VitalityLayouts.end();
}

const VitalityLayout* State_VitalityBuilder::GetLayout(
    const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    auto it = m_VitalityLayouts.find(tagName);
    return (it != m_VitalityLayouts.end()) ? &it->second : nullptr;
}

void State_VitalityBuilder::AddLayout(
    const std::string& tagName, VitalityLayout layout)
{
    assert(!m_Frozen.load(std::memory_order_relaxed));
    m_VitalityLayouts.emplace(tagName, std::move(layout));
}

void State_VitalityBuilder::Cleanup()
{
    m_Frozen.store(false, std::memory_order_relaxed);
    m_VitalityLayouts.clear();
}