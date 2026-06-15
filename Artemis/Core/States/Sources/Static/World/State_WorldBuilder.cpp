#include "pch.h"

#include "State_WorldBuilder.h"

// Coll.
bool State_WorldBuilder::HasCollGeometry(const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));

    auto it = m_CollGeometries.find(tagName);
    if (it == m_CollGeometries.end())
    {
        return false;
    }
    return true;
}

const CollGeometry* State_WorldBuilder::GetCollGeometry(
    const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));

    auto it = m_CollGeometries.find(tagName);
    if (it == m_CollGeometries.end())
    {
        return {};
    }
    return &it->second;
}

void State_WorldBuilder::AddCollGeometry(
    const std::string& tagName, CollGeometry geometry)
{
    assert(!m_Frozen.load(std::memory_order_relaxed));
    m_CollGeometries.emplace(tagName, std::move(geometry));
}

// Mode.
bool State_WorldBuilder::HasModeGeometry(const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));

    auto it = m_ModeGeometries.find(tagName);
    if (it == m_ModeGeometries.end())
    {
        return false;
    }
    return true;
}

const ModeGeometry* State_WorldBuilder::GetModeGeometry(
    const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));

    auto it = m_ModeGeometries.find(tagName);
    if (it == m_ModeGeometries.end())
    {
        return nullptr;
    }
    return &it->second;
}

void State_WorldBuilder::AddModeGeometry(
    const std::string& tagName, ModeGeometry geometry)
{
    assert(!m_Frozen.load(std::memory_order_relaxed));
    m_ModeGeometries.emplace(tagName, std::move(geometry));
}

// Sbsp.
bool State_WorldBuilder::HasSbspGeometry() const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    return !m_SbspGeometries.empty();
}

const SbspGeometry* State_WorldBuilder::GetSbspGeometry(int32_t index) const
{
    assert(m_Frozen.load(std::memory_order_acquire));

    if (index < 0 || index >= static_cast<int32_t>(m_SbspGeometries.size()))
    {
        return nullptr;
    }
    return &m_SbspGeometries[index];
}

void State_WorldBuilder::AddSbspGeometry(SbspGeometry geometry)
{
    assert(!m_Frozen.load(std::memory_order_relaxed));
    m_SbspGeometries.push_back(std::move(geometry));
}

const SbspGeometries& State_WorldBuilder::GetAllSbspGeometry() const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    return m_SbspGeometries;
}

int32_t State_WorldBuilder::GetSbspGeometryCount() const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    return static_cast<int32_t>(m_SbspGeometries.size());
}

// Navigation graph.
const std::vector<NavigationGraph>& State_WorldBuilder::GetNavigationGraph() const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    return m_NavigationGraphs;
}

void State_WorldBuilder::SetNavigationGraph(std::vector<NavigationGraph> clusters)
{
    assert(!m_Frozen.load(std::memory_order_relaxed));
    m_NavigationGraphs = std::move(clusters);
}

size_t State_WorldBuilder::GetNodeCount(const std::string& tagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));

    if (auto it = m_ModeGeometries.find(tagName); it != m_ModeGeometries.end())
    {
        return it->second.Nodes.size();
    }

    if (auto it = m_CollGeometries.find(tagName); it != m_CollGeometries.end())
    {
        return it->second.Nodes.size();
    }

    return 0;
}

void State_WorldBuilder::LinkObjectColl(
    const std::string& objectTagName, const std::string& collTagName)
{
    assert(!m_Frozen.load(std::memory_order_relaxed));
    m_ObjectColls[objectTagName] = collTagName;
}

std::string State_WorldBuilder::ResolveObjectCollName(
    const std::string& objectTagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    auto it = m_ObjectColls.find(objectTagName);
    return it != m_ObjectColls.end() ? it->second : std::string{};
}

void State_WorldBuilder::LinkObjectHlmt(
    const std::string& objectTagName, const std::string& hlmtTagName)
{
    assert(!m_Frozen.load(std::memory_order_relaxed));
    m_ObjectHlmts[objectTagName] = hlmtTagName;
}

std::string State_WorldBuilder::ResolveObjectHlmtName(
    const std::string& objectTagName) const
{
    assert(m_Frozen.load(std::memory_order_acquire));
    auto it = m_ObjectHlmts.find(objectTagName);
    return it != m_ObjectHlmts.end() ? it->second : std::string{};
}

void State_WorldBuilder::Cleanup()
{
    m_Frozen.store(false, std::memory_order_relaxed);

    m_ObjectColls.clear();

    m_CollGeometries.clear();
    m_ModeGeometries.clear();
    m_SbspGeometries.clear();
    m_NavigationGraphs.clear();
}