module;

#include <cassert>

module Resolved.Stats.State;

namespace Resolved::Stats::State
{
    // Vehi.
    auto StatsStore::HasResolvedVehi(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedVehis.find(tagName) != m_ResolvedVehis.end();
    }

    auto StatsStore::GetResolvedVehi(const std::string& tagName) const -> const ResolvedVehi*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedVehis.find(tagName);
        if (it == m_ResolvedVehis.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto StatsStore::AddResolvedVehi(const std::string& tagName, ResolvedVehi data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedVehis.emplace(tagName, std::move(data));
    }

    // Weap.
    auto StatsStore::HasResolvedWeap(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedWeaps.find(tagName) != m_ResolvedWeaps.end();
    }

    auto StatsStore::GetResolvedWeap(const std::string& tagName) const -> const ResolvedWeap*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedWeaps.find(tagName);
        if (it == m_ResolvedWeaps.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto StatsStore::AddResolvedWeap(const std::string& tagName, ResolvedWeap data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedWeaps.emplace(tagName, std::move(data));
    }

    // Proj.
    auto StatsStore::HasResolvedProj(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedProjs.find(tagName) != m_ResolvedProjs.end();
    }

    auto StatsStore::GetResolvedProj(const std::string& tagName) const -> const ResolvedProj*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedProjs.find(tagName);
        if (it == m_ResolvedProjs.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto StatsStore::AddResolvedProj(const std::string& tagName, ResolvedProj data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedProjs.emplace(tagName, std::move(data));
    }

    auto StatsStore::Cleanup() -> void
    {
        m_Frozen.store(false, std::memory_order_relaxed);

        m_ResolvedVehis.clear();
        m_ResolvedWeaps.clear();
        m_ResolvedProjs.clear();
    }
}