module;

#include <cassert>

module Resolved.Vitality.State;

namespace Resolved::Vitality::State
{
    auto VitalityStore::HasResolvedVitality(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedVitalities.find(tagName) != m_ResolvedVitalities.end();
    }

    auto VitalityStore::GetResolvedVitality(const std::string& tagName) const -> const ResolvedVitality*
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        auto it = m_ResolvedVitalities.find(tagName);
        return (it != m_ResolvedVitalities.end()) ? it->second.get() : nullptr;
    }

    auto VitalityStore::GetSharedVitality(const std::string& tagName) const -> std::shared_ptr<const ResolvedVitality>
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        auto it = m_ResolvedVitalities.find(tagName);
        return (it != m_ResolvedVitalities.end()) ? it->second : nullptr;
    }

    auto VitalityStore::AddResolvedVitality(const std::string& tagName, ResolvedVitality layout) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedVitalities.emplace(tagName, std::make_shared<const ResolvedVitality>(std::move(layout)));
    }

    auto VitalityStore::Cleanup() -> void
    {
        m_Frozen.store(false, std::memory_order_relaxed);
        m_ResolvedVitalities.clear();
    }
}