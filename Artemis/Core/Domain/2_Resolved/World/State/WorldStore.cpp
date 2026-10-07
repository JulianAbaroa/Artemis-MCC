module;

#include <cassert>

module Resolved.World.State;

namespace Resolved::World::State
{
    auto WorldStore::GetCollForObject(const std::string& objectTagName,
        const DefinitionsStore& definitionsStore) const -> const Coll*
    {
        assert(m_IsFrozen.load(std::memory_order_acquire));

        auto it = m_ObjectColls.find(objectTagName);
        if (it == m_ObjectColls.end())
        {
            return nullptr;
        }
        return definitionsStore.Coll.Get(it->second);
    }

    auto WorldStore::GetRegionStates(
        const std::string& objectTagName) const -> const RegionStates*
    {
        assert(m_IsFrozen.load(std::memory_order_acquire));

        auto it = m_ObjectHlmtIndex.find(objectTagName);
        if (it == m_ObjectHlmtIndex.end())
        {
            return nullptr;
        }
        return &m_RegionStates[it->second];
    }

    auto WorldStore::AddRegionStates(const std::string& hlmtTagName,
        RegionStates states) -> void
    {
        assert(!m_IsFrozen.load(std::memory_order_acquire));

        const std::int32_t index{ static_cast<std::int32_t>(m_RegionStates.size()) };
        m_RegionStates.push_back(std::move(states));
        m_RegionStatesIndexByName.emplace(hlmtTagName, index);
    }

    auto WorldStore::GetModelLink(const std::string& hlmtTagName) const -> const ModelLink*
    {
        assert(m_IsFrozen.load(std::memory_order_acquire));

        auto it = m_ModelLinks.find(hlmtTagName);
        return (it != m_ModelLinks.end()) ? &it->second : nullptr;
    }

    auto WorldStore::AddModelLink(const std::string& hlmtTagName, ModelLink link) -> void
    {
        assert(!m_IsFrozen.load(std::memory_order_acquire));

        m_ModelLinks.emplace(hlmtTagName, std::move(link));
    }

    auto WorldStore::ResolveObjectCollName(const std::string& objectTagName) const -> std::string
    {
        assert(m_IsFrozen.load(std::memory_order_acquire));

        auto it = m_ObjectColls.find(objectTagName);
        return it != m_ObjectColls.end() ? it->second : std::string{};
    }

    auto WorldStore::LinkObjectColl(const std::string& objectTagName,
        const std::string& collTagName) -> void
    {
        assert(!m_IsFrozen.load(std::memory_order_relaxed));

        m_ObjectColls[objectTagName] = collTagName;
    }

    auto WorldStore::ResolveObjectHlmtName(const std::string& objectTagName) const -> std::string
    {
        assert(m_IsFrozen.load(std::memory_order_acquire));

        auto it = m_ObjectHlmts.find(objectTagName);
        return it != m_ObjectHlmts.end() ? it->second : std::string{};
    }

    auto WorldStore::LinkObjectHlmt(const std::string& objectTagName,
        const std::string& hlmtTagName) -> void
    {
        assert(!m_IsFrozen.load(std::memory_order_relaxed));

        m_ObjectHlmts[objectTagName] = hlmtTagName;
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
        m_IsFrozen.store(false, std::memory_order_relaxed);

        m_ModelLinks.clear();

        m_ObjectColls.clear();
        m_ObjectHlmts.clear();
        m_ObjectHlmtIndex.clear();

        m_RegionStates.clear();
        m_RegionStatesIndexByName.clear();
    }
}