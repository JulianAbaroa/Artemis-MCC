module;

#include <cassert>

module Resolved.Vitality.State;

namespace Resolved::Vitality::State
{
    auto VitalityStore::Has(const std::string& tagName) const -> bool
    {
        assert(m_IsFrozen.load(std::memory_order_acquire));
        return m_Vitalities.find(tagName) != m_Vitalities.end();
    }

    auto VitalityStore::Get(const std::string& tagName) const -> const Vitality*
    {
        assert(m_IsFrozen.load(std::memory_order_acquire));
        auto it = m_Vitalities.find(tagName);
        return (it != m_Vitalities.end()) ? it->second.get() : nullptr;
    }

    auto VitalityStore::GetShared(const std::string& tagName) const -> std::shared_ptr<const Vitality>
    {
        assert(m_IsFrozen.load(std::memory_order_acquire));
        auto it = m_Vitalities.find(tagName);
        return (it != m_Vitalities.end()) ? it->second : nullptr;
    }

    auto VitalityStore::Add(const std::string& tagName, Vitality layout) -> void
    {
        assert(!m_IsFrozen.load(std::memory_order_relaxed));
        m_Vitalities.emplace(tagName, std::make_shared<const Vitality>(std::move(layout)));
    }

    auto VitalityStore::Cleanup() -> void
    {
        m_IsFrozen.store(false, std::memory_order_relaxed);
        m_Vitalities.clear();
    }
}