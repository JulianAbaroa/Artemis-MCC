module;

#include <cassert>

module Resolved.World.State;

namespace Resolved::World::State
{
    // Coll.
    auto WorldStore::HasResolvedColl(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedColls.find(tagName);
        if (it == m_ResolvedColls.end())
        {
            return false;
        }
        return true;
    }

    auto WorldStore::GetResolvedColl(const std::string& tagName) const -> const ResolvedColl*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedColls.find(tagName);
        if (it == m_ResolvedColls.end())
        {
            return {};
        }
        return &it->second;
    }

    auto WorldStore::AddResolvedColl(const std::string& tagName, ResolvedColl geometry) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedColls.emplace(tagName, std::move(geometry));
    }

    auto WorldStore::PeekResolvedColl(const std::string& tagName) const -> const ResolvedColl*
    {
        auto it = m_ResolvedColls.find(tagName);
        return it != m_ResolvedColls.end() ? &it->second : nullptr;
    }

    auto WorldStore::GetResolvedCollForObject(
        const std::string& objectTagName) const -> const ResolvedColl*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto object = m_ObjectColls.find(objectTagName);
        if (object == m_ObjectColls.end()) return nullptr;

        auto it = m_ResolvedColls.find(object->second);
        return it != m_ResolvedColls.end() ? &it->second : nullptr;
    }

    auto WorldStore::GetResolvedRegionStates(
        const std::string& objectTagName) const -> const ResolvedRegionStates*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto object = m_ObjectHlmts.find(objectTagName);
        if (object == m_ObjectHlmts.end())
        {
            return nullptr;
        }

        auto it = m_ResolvedRegionStates.find(object->second);
        if (it == m_ResolvedRegionStates.end())
        {
            return nullptr;
        }

        return &it->second;
    }

    auto WorldStore::AddResolvedRegionStates(const std::string& hlmtTagName,
        ResolvedRegionStates states) -> void
    {
        assert(!m_Frozen.load(std::memory_order_acquire));
        m_ResolvedRegionStates.emplace(hlmtTagName, std::move(states));
    }

    // Mode.
    auto WorldStore::HasResolvedMode(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedModes.find(tagName);
        if (it == m_ResolvedModes.end())
        {
            return false;
        }
        return true;
    }

    auto WorldStore::GetResolvedMode(const std::string& tagName) const -> const ResolvedMode*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedModes.find(tagName);
        if (it == m_ResolvedModes.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto WorldStore::AddResolvedMode(const std::string& tagName, ResolvedMode geometry) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedModes.emplace(tagName, std::move(geometry));
    }

    // Sbsp.
    auto WorldStore::HasResolvedSbsp() const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return !m_ResolvedSbsps.empty();
    }

    auto WorldStore::GetResolvedSbsp(std::int32_t index) const -> const ResolvedSbsp*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        if (index < 0 || index >= static_cast<std::int32_t>(m_ResolvedSbsps.size()))
        {
            return nullptr;
        }
        return &m_ResolvedSbsps[index];
    }

    auto WorldStore::AddResolvedSbsp(ResolvedSbsp geometry) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedSbsps.push_back(std::move(geometry));
    }

    auto WorldStore::GetAllResolvedSbsps() const -> const std::vector<ResolvedSbsp>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedSbsps;
    }

    auto WorldStore::GetResolvedSbspCount() const -> std::int32_t
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return static_cast<std::int32_t>(m_ResolvedSbsps.size());
    }

    auto WorldStore::GetNodeCount(const std::string& tagName) const -> std::size_t
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        if (auto it = m_ResolvedModes.find(tagName); it != m_ResolvedModes.end())
        {
            return it->second.Nodes.size();
        }

        if (auto it = m_ResolvedColls.find(tagName); it != m_ResolvedColls.end())
        {
            return it->second.Nodes.size();
        }

        return 0;
    }

    auto WorldStore::ResolveObjectCollName(
        const std::string& objectTagName) const -> std::string
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        auto it = m_ObjectColls.find(objectTagName);
        return it != m_ObjectColls.end() ? it->second : std::string{};
    }

    auto WorldStore::LinkObjectColl(const std::string& objectTagName,
        const std::string& collTagName) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ObjectColls[objectTagName] = collTagName;
    }

    auto WorldStore::LinkObjectHlmt(const std::string& objectTagName,
        const std::string& hlmtTagName) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ObjectHlmts[objectTagName] = hlmtTagName;
    }

    auto WorldStore::ResolveObjectHlmtName(
        const std::string& objectTagName) const -> std::string
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        auto it = m_ObjectHlmts.find(objectTagName);
        return it != m_ObjectHlmts.end() ? it->second : std::string{};
    }

    auto WorldStore::Cleanup() -> void
    {
        m_Frozen.store(false, std::memory_order_relaxed);

        m_ObjectColls.clear();
        m_ObjectHlmts.clear();

        m_ResolvedColls.clear();
        m_ResolvedModes.clear();
        m_ResolvedSbsps.clear();
        m_ResolvedRegionStates.clear();
    }
}