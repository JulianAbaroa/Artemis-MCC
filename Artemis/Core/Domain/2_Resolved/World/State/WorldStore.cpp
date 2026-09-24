module;

#include <cassert>

module Resolved.World.State;

namespace Resolved::World::State
{
    // Coll.
    auto WorldStore::HasResolvedColl(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_CollIndexByName.find(tagName) != m_CollIndexByName.end();
    }

    auto WorldStore::GetResolvedColl(const std::string& tagName) const -> const ResolvedColl*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_CollIndexByName.find(tagName);
        if (it == m_CollIndexByName.end())
        {
            return nullptr;
        }
        return &m_ResolvedColls[it->second];
    }

    auto WorldStore::AddResolvedColl(const std::string& tagName, ResolvedColl geometry) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));

        const std::int32_t index = static_cast<std::int32_t>(m_ResolvedColls.size());
        m_ResolvedColls.push_back(std::move(geometry));
        m_CollIndexByName.emplace(tagName, index);
    }

    auto WorldStore::PeekResolvedColl(const std::string& tagName) const -> const ResolvedColl*
    {
        auto it = m_CollIndexByName.find(tagName);
        return it != m_CollIndexByName.end() ? &m_ResolvedColls[it->second] : nullptr;
    }

    auto WorldStore::GetResolvedCollForObject(
        const std::string& objectTagName) const -> const ResolvedColl*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ObjectCollIndex.find(objectTagName);
        if (it == m_ObjectCollIndex.end())
        {
            return nullptr;
        }
        return &m_ResolvedColls[it->second];
    }

    auto WorldStore::GetResolvedRegionStates(
        const std::string& objectTagName) const -> const ResolvedRegionStates*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ObjectHlmtIndex.find(objectTagName);
        if (it == m_ObjectHlmtIndex.end())
        {
            return nullptr;
        }
        return &m_ResolvedRegionStates[it->second];
    }

    auto WorldStore::AddResolvedRegionStates(const std::string& hlmtTagName,
        ResolvedRegionStates states) -> void
    {
        assert(!m_Frozen.load(std::memory_order_acquire));

        const std::int32_t index = static_cast<std::int32_t>(m_ResolvedRegionStates.size());
        m_ResolvedRegionStates.push_back(std::move(states));
        m_RegionStatesIndexByName.emplace(hlmtTagName, index);
    }

    // Mode.
    auto WorldStore::HasResolvedMode(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ModeIndexByName.find(tagName) != m_ModeIndexByName.end();
    }

    auto WorldStore::GetResolvedMode(const std::string& tagName) const -> const ResolvedMode*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ModeIndexByName.find(tagName);
        if (it == m_ModeIndexByName.end())
        {
            return nullptr;
        }
        return &m_ResolvedModes[it->second];
    }

    auto WorldStore::AddResolvedMode(const std::string& tagName, ResolvedMode geometry) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));

        const std::int32_t index = static_cast<std::int32_t>(m_ResolvedModes.size());
        m_ResolvedModes.push_back(std::move(geometry));
        m_ModeIndexByName.emplace(tagName, index);
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

        if (auto it = m_ModeIndexByName.find(tagName); it != m_ModeIndexByName.end())
        {
            return m_ResolvedModes[it->second].Nodes.size();
        }

        if (auto it = m_CollIndexByName.find(tagName); it != m_CollIndexByName.end())
        {
            return m_ResolvedColls[it->second].Nodes.size();
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

    auto WorldStore::CompileObjectIndices() -> void
    {
        m_ObjectCollIndex.clear();
        m_ObjectCollIndex.reserve(m_ObjectColls.size());

        for (const auto& [objectTagName, collTagName] : m_ObjectColls)
        {
            auto it = m_CollIndexByName.find(collTagName);
            if (it == m_CollIndexByName.end()) continue;

            m_ObjectCollIndex.emplace(objectTagName, it->second);
        }

        m_ObjectHlmtIndex.clear();
        m_ObjectHlmtIndex.reserve(m_ObjectHlmts.size());

        for (const auto& [objectTagName, hlmtTagName] : m_ObjectHlmts)
        {
            auto it = m_RegionStatesIndexByName.find(hlmtTagName);
            if (it == m_RegionStatesIndexByName.end()) continue;

            m_ObjectHlmtIndex.emplace(objectTagName, it->second);
        }
    }

    auto WorldStore::Freeze() -> void
    {
        this->CompileObjectIndices();
        m_Frozen.store(true, std::memory_order_release);
    }

    auto WorldStore::Cleanup() -> void
    {
        m_Frozen.store(false, std::memory_order_relaxed);

        m_ObjectColls.clear();
        m_ObjectHlmts.clear();
        m_ObjectCollIndex.clear();
        m_ObjectHlmtIndex.clear();

        m_ResolvedColls.clear();
        m_CollIndexByName.clear();

        m_ResolvedModes.clear();
        m_ModeIndexByName.clear();

        m_ResolvedSbsps.clear();

        m_ResolvedRegionStates.clear();
        m_RegionStatesIndexByName.clear();
    }
}