module;

#include <cassert>

module Resolved.World.State;

namespace Resolved::World::State
{
    // Coll.
    auto WorldStore::GetResolvedCollForObject(const std::string& objectTagName,
        const DefinitionsStore& definitionsStore) const -> const ResolvedColl*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ObjectColls.find(objectTagName);
        if (it == m_ObjectColls.end())
        {
            return nullptr;
        }
        return definitionsStore.GetResolvedColl(it->second);
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

    auto WorldStore::GetResolvedModelLink(const std::string& hlmtTagName) const -> const ResolvedModelLink*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ModelLinks.find(hlmtTagName);
        return (it != m_ModelLinks.end()) ? &it->second : nullptr;
    }

    auto WorldStore::AddResolvedModelLink(const std::string& hlmtTagName, ResolvedModelLink link) -> void
    {
        assert(!m_Frozen.load(std::memory_order_acquire));

        m_ModelLinks.emplace(hlmtTagName, std::move(link));
    }

    auto WorldStore::GetNodeCount(const std::string& tagName,
        const DefinitionsStore& definitionsStore) const -> std::size_t
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        if (const auto* mode = definitionsStore.GetResolvedMode(tagName))
        {
            return mode->Nodes.size();
        }

        if (const ResolvedColl* coll = definitionsStore.GetResolvedColl(tagName))
        {
            return coll->Nodes.size();
        }

        return 0;
    }

    auto WorldStore::ResolveObjectCollName(const std::string& objectTagName) const -> std::string
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

    auto WorldStore::ResolveObjectHlmtName(const std::string& objectTagName) const -> std::string
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        auto it = m_ObjectHlmts.find(objectTagName);
        return it != m_ObjectHlmts.end() ? it->second : std::string{};
    }

    auto WorldStore::CompileObjectIndices() -> void
    {
        m_ObjectHlmtIndex.clear();
        m_ObjectHlmtIndex.reserve(m_ObjectHlmts.size());

        for (const auto& [objectTagName, hlmtTagName] : m_ObjectHlmts)
        {
            auto it = m_RegionStatesIndexByName.find(hlmtTagName);
            if (it == m_RegionStatesIndexByName.end()) continue;

            m_ObjectHlmtIndex.emplace(objectTagName, it->second);
        }
    }

    auto WorldStore::Cleanup() -> void
    {
        m_Frozen.store(false, std::memory_order_relaxed);

        m_ModelLinks.clear();

        m_ObjectColls.clear();
        m_ObjectHlmts.clear();
        m_ObjectHlmtIndex.clear();

        m_ResolvedRegionStates.clear();
        m_RegionStatesIndexByName.clear();
    }
}