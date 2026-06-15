#include "pch.h"

#include "State_StatsBuilder.h"

// Proj.
bool State_StatsBuilder::HasProj(const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    return m_Projs.find(tagName) != m_Projs.end();
}

const Proj* State_StatsBuilder::GetProj(
    const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));

    auto it = m_Projs.find(tagName);
    if (it == m_Projs.end())
    {
        return nullptr;
    }
    return &it->second;
}

void State_StatsBuilder::AddProj(
    const std::string& tagName, Proj data)
{
    assert(!m_Frozen.load(std::memory_order_relaxed));
    m_Projs.emplace(tagName,std::move(data));
}

// Vehi.
bool State_StatsBuilder::HasVehi(const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    return m_Vehis.find(tagName) != m_Vehis.end();
}

const Vehi* State_StatsBuilder::GetVehi(
    const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));

    auto it = m_Vehis.find(tagName);
    if (it == m_Vehis.end())
    {
        return nullptr;
    }
    return &it->second;
}

void State_StatsBuilder::AddVehi(
    const std::string& tagName, Vehi data)
{
    assert(!m_Frozen.load(std::memory_order_relaxed));
    m_Vehis.emplace(tagName, std::move(data));
}

// Weap.
bool State_StatsBuilder::HasWeap(const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    return m_Weaps.find(tagName) != m_Weaps.end();
}

const Weap* State_StatsBuilder::GetWeap(
    const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));

    auto it = m_Weaps.find(tagName);
    if (it == m_Weaps.end())
    {
        return nullptr;
    }
    return &it->second;
}

void State_StatsBuilder::AddWeap(
    const std::string& tagName, Weap data)
{
    assert(!m_Frozen.load(std::memory_order_relaxed));
    m_Weaps.emplace(tagName, std::move(data));
}

void State_StatsBuilder::Cleanup()
{
    m_Frozen.store(false, std::memory_order_relaxed);

    m_Projs.clear();
    m_Vehis.clear();
    m_Weaps.clear();
}